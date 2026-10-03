#include <gtest/gtest.h>
#include "logos_core.h"
#include "dummy_module_generator.h"
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStringList>
#include <QTemporaryDir>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Reusable barrier so all threads start work at the same instant.
class Barrier {
public:
    explicit Barrier(int count) : m_threshold(count), m_waiting(0), m_generation(0) {}

    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        int gen = m_generation;
        if (++m_waiting == m_threshold) {
            m_generation++;
            m_waiting = 0;
            m_cv.notify_all();
        } else {
            m_cv.wait(lock, [&] { return gen != m_generation; });
        }
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    int m_threshold;
    int m_waiting;
    int m_generation;
};

// =============================================================================
// The runtime. Loads, unloads and queries go through core_service over the
// shell binding, as an embedder's do; only logos_core_process_module stays on
// the C API. liblogos hosts its token authority (capability_module) in-process
// once per process, so every test shares one runtime and names its own modules.
// =============================================================================

// Qt requires at least one argument (the program name) for QCoreApplication.
static int    s_argc    = 1;
static char   s_name[]  = "thread_safety_tests";
static char*  s_argv[]  = {s_name, nullptr};

static logos_consumer* s_shell = nullptr;

// core_service deadlines, as logos::host::LogosCore's: a load waits out its host's bring-up.
constexpr int kLifecycleMs = 120000;
constexpr int kQueryMs = 15000;

// The runtime's own modules: liblogos' modules/, where capability_module ships.
static QByteArray bundledModulesDir() {
    const QByteArray env = qgetenv("LOGOS_BUNDLED_MODULES_DIR");
    if (!env.isEmpty()) return env;
#ifdef LOGOS_BUNDLED_MODULES_DIR
    return QByteArray(LOGOS_BUNDLED_MODULES_DIR);
#else
    return {};
#endif
}

class RuntimeEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        const QByteArray bundled = bundledModulesDir();
        ASSERT_FALSE(bundled.isEmpty())
            << "set LOGOS_BUNDLED_MODULES_DIR to liblogos' modules/ (capability_module)";
        logos_core_init(s_argc, s_argv);
        m_initialized = true;
        const char* dirs[] = {bundled.constData(), nullptr};
        ASSERT_EQ(logos_core_set_bundled_modules_dirs(dirs), 0);
        ASSERT_EQ(logos_core_set_shell_identity(s_name), 0);
        logos_core_start();
        s_shell = logos_core_take_shell_binding();
        ASSERT_NE(s_shell, nullptr)
            << "no shell binding: no token authority in " << bundled.constData();
    }

    void TearDown() override {
        if (s_shell) logos_consumer_release(s_shell);
        s_shell = nullptr;
        if (m_initialized) logos_core_cleanup();
    }

private:
    bool m_initialized = false;
};

// gtest owns it and runs it around every test.
[[maybe_unused]] static ::testing::Environment* const s_runtime =
    ::testing::AddGlobalTestEnvironment(new RuntimeEnvironment);

// One core_service call: its answer, or null when the call itself failed.
static QJsonValue coreService(const char* method, const QJsonArray& args, int timeoutMs) {
    char* result = nullptr;
    char* error = nullptr;
    const QByteArray argsJson = QJsonDocument(args).toJson(QJsonDocument::Compact);
    const int status = logos_consumer_call(s_shell, "core_service", method, argsJson.constData(),
                                           timeoutMs, &result, &error);
    QJsonValue answer;
    if (status == 0 && result) {
        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(result));
        if (doc.isArray()) answer = doc.array();
        else if (doc.isObject()) answer = doc.object();
    }
    logos_consumer_string_free(result);
    logos_consumer_string_free(error);
    return answer;
}

// `deps` is "module_only" or "required", the two LogosLoadDeps modes this suite loads with.
static QJsonObject loadAnswer(const std::string& name, const char* deps) {
    return coreService("loadModule", {QString::fromStdString(name), QString::fromLatin1(deps)},
                       kLifecycleMs).toObject();
}

static bool loadModule(const std::string& name, const char* deps) {
    return loadAnswer(name, deps).value("status").toString() == QLatin1String("ok");
}

static bool unloadModule(const std::string& name) {
    return coreService("unloadModule", {QString::fromStdString(name), false}, kLifecycleMs)
               .toObject().value("status").toString() == QLatin1String("ok");
}

// listModules: "all" names every known module, "loaded" the loaded ones.
static QStringList moduleNames(const char* filter, bool* answered = nullptr) {
    const QJsonValue answer = coreService("listModules", {QString::fromLatin1(filter)}, kQueryMs);
    if (answered) *answered = answer.isArray();
    QStringList names;
    for (const QJsonValue& entry : answer.toArray())
        names << entry.toObject().value("name").toString();
    return names;
}

static QStringList knownModules() { return moduleNames("all"); }
static QStringList loadedModules() { return moduleNames("loaded"); }

// How many entries of `names` are this test's modules; a duplicate counts twice.
static int countOf(const QStringList& names, const QVector<DummyModule>& mine) {
    QSet<QString> own;
    for (const DummyModule& m : mine) own.insert(m.name);
    int n = 0;
    for (const QString& name : names)
        if (own.contains(name)) ++n;
    return n;
}

// This test's loaded-module count, read once a host killed along with the thread
// that loaded it would have been reaped (that takes milliseconds).
static int loadedCountAfterSettling(const QVector<DummyModule>& mine) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    return countOf(loadedModules(), mine);
}

// =============================================================================
// Lightweight tests — no real plugin files needed. Load names that are unknown
// to the registry.
// =============================================================================

class PluginApiTest : public ::testing::Test {
protected:
    void SetUp() override { ASSERT_NE(s_shell, nullptr); }

    static constexpr int kThreads = 8;
    static constexpr int kIterations = 200;
};

// -----------------------------------------------------------------------------
// Multiple threads all try to load unknown modules concurrently. Every call
// must be answered with a failed load, without crashing.
// -----------------------------------------------------------------------------
TEST_F(PluginApiTest, ConcurrentLoadUnknownPlugins) {
    Barrier barrier(kThreads);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&barrier, t]() {
            barrier.wait();
            for (int i = 0; i < kIterations; ++i) {
                std::string name = "unknown_" + std::to_string(t) + "_" + std::to_string(i);
                EXPECT_EQ(loadAnswer(name, "module_only").value("code").toString().toStdString(),
                          "MODULE_LOAD_FAILED") << name;
            }
        });
    }

    for (auto& th : threads) th.join();
}

// -----------------------------------------------------------------------------
// Loads of unknown modules with their required dependencies, from many threads.
// Every call must be answered with a failed load, without crashing.
// -----------------------------------------------------------------------------
TEST_F(PluginApiTest, ConcurrentLoadWithDepsUnknown) {
    Barrier barrier(kThreads);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&barrier, t]() {
            barrier.wait();
            for (int i = 0; i < kIterations; ++i) {
                std::string name = "nodeps_" + std::to_string(t) + "_" + std::to_string(i);
                EXPECT_EQ(loadAnswer(name, "required").value("code").toString().toStdString(),
                          "MODULE_LOAD_FAILED") << name;
            }
        });
    }

    for (auto& th : threads) th.join();
}

// =============================================================================
// Real-plugin tests — use binary-patched copies of a real Qt plugin.
// =============================================================================

class RealPluginThreadSafetyTest : public ::testing::Test {
protected:
    static constexpr int kThreads = 8;
    static constexpr int kModuleCount = 100;
    static constexpr int kIterations = 200;

    QTemporaryDir tmpDir;
    QVector<DummyModule> modules;

    void SetUp() override {
        ASSERT_NE(s_shell, nullptr);
        ASSERT_TRUE(tmpDir.isValid());
        // Names no earlier test used: the runtime still knows theirs.
        static int s_nextIndex = 0;
        const int firstIndex = s_nextIndex;
        s_nextIndex += kModuleCount;
        modules = DummyModuleGenerator::generate(kModuleCount, tmpDir.path(), firstIndex);
        if (modules.isEmpty())
            GTEST_SKIP() << "Dummy plugin template not found — skipping real-plugin tests";
        ASSERT_EQ(modules.size(), kModuleCount) << "Partial plugin generation — expected "
            << kModuleCount << " but got " << modules.size();
    }

    // Stops this test's hosts, so the next test starts from none.
    void TearDown() override {
        if (!s_shell) return;
        const QStringList loaded = loadedModules();
        for (const DummyModule& m : modules) {
            if (loaded.contains(m.name))
                EXPECT_TRUE(unloadModule(m.name.toStdString())) << m.name.toStdString();
        }
    }

    void processAll(int count) {
        for (int i = 0; i < count; ++i) {
            std::string path = modules[i].path.toStdString();
            char* name = logos_core_process_module(path.c_str());
            delete[] name;
        }
    }
};

// -----------------------------------------------------------------------------
// Two generated copies load side by side, each under its own name. The load
// tests below ignore load results, so a copy the host refuses ("plugin name
// mismatch") would otherwise pass unnoticed.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, GeneratedCopiesLoadUnderTheirOwnNames) {
    const QVector<DummyModule> pair = {modules[1], modules[2]};
    for (const DummyModule& m : pair) {
        std::string path = m.path.toStdString();
        char* name = logos_core_process_module(path.c_str());
        ASSERT_NE(name, nullptr) << path;
        EXPECT_EQ(std::string(name), m.name.toStdString());
        delete[] name;
    }

    for (const DummyModule& m : pair) {
        std::string name = m.name.toStdString();
        EXPECT_TRUE(loadModule(name, "module_only")) << name;
    }

    const QStringList loaded = loadedModules();
    for (const DummyModule& m : pair)
        EXPECT_TRUE(loaded.contains(m.name)) << m.name.toStdString();
}

// -----------------------------------------------------------------------------
// Each thread processes a disjoint slice of the generated plugins via
// logos_core_process_module. After joining, every plugin must be known.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentProcessPlugins) {
    Barrier barrier(kThreads);
    std::atomic<int> processed{0};
    std::vector<std::thread> threads;

    int perThread = kModuleCount / kThreads;

    for (int t = 0; t < kThreads; ++t) {
        int start = t * perThread;
        int end = (t == kThreads - 1) ? kModuleCount : start + perThread;

        threads.emplace_back([&, start, end]() {
            barrier.wait();
            for (int i = start; i < end; ++i) {
                std::string path = modules[i].path.toStdString();
                char* name = logos_core_process_module(path.c_str());
                if (name) {
                    processed.fetch_add(1, std::memory_order_relaxed);
                    delete[] name;
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(processed.load(), kModuleCount);

    const QStringList known = knownModules();
    EXPECT_EQ(countOf(known, modules), kModuleCount);
    for (const DummyModule& m : modules)
        EXPECT_TRUE(known.contains(m.name)) << m.name.toStdString();
}

// -----------------------------------------------------------------------------
// All threads try to process the SAME set of plugins — tests idempotent
// insertion under heavy write contention.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentProcessSamePlugins) {
    Barrier barrier(kThreads);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            barrier.wait();
            for (const DummyModule& m : modules) {
                std::string path = m.path.toStdString();
                char* name = logos_core_process_module(path.c_str());
                delete[] name;
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(countOf(knownModules(), modules), kModuleCount);
}

// -----------------------------------------------------------------------------
// Half the threads process plugins while the other half keep listing the known
// modules. Tests reader safety during concurrent writes.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentProcessWhileQuerying) {
    Barrier barrier(kThreads);
    std::atomic<int> writersDone{0};
    std::vector<std::thread> threads;

    int writers = kThreads / 2;

    for (int t = 0; t < writers; ++t) {
        int start = t * (kModuleCount / writers);
        int end = (t == writers - 1) ? kModuleCount : start + (kModuleCount / writers);

        threads.emplace_back([&, start, end]() {
            barrier.wait();
            for (int i = start; i < end; ++i) {
                std::string path = modules[i].path.toStdString();
                char* name = logos_core_process_module(path.c_str());
                delete[] name;
            }
            writersDone.fetch_add(1, std::memory_order_release);
        });
    }

    for (int t = writers; t < kThreads; ++t) {
        threads.emplace_back([&, writers]() {
            barrier.wait();
            while (writersDone.load(std::memory_order_acquire) < writers) {
                bool answered = false;
                (void)moduleNames("all", &answered);
                EXPECT_TRUE(answered);
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(countOf(knownModules(), modules), kModuleCount);
}

// -----------------------------------------------------------------------------
// The known and loaded lists are read concurrently while another thread
// repeatedly loads and unloads a small set of plugins. Tests that the list
// accessors are safe under concurrent state changes.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentGetListsDuringLoadUnload) {
    constexpr int kSmall = kThreads;
    processAll(kSmall);

    Barrier barrier(kThreads);
    std::atomic<bool> done{false};
    std::vector<std::thread> threads;

    // Writer thread repeatedly loads then unloads the same small set.
    threads.emplace_back([&]() {
        barrier.wait();
        for (int iter = 0; iter < kIterations; ++iter) {
            std::string name = modules[iter % kSmall].name.toStdString();
            (void)loadModule(name, "module_only");
            (void)unloadModule(name);
        }
        done.store(true, std::memory_order_release);
    });

    // Reader threads read both lists in a tight loop.
    for (int t = 1; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            barrier.wait();
            while (!done.load(std::memory_order_acquire)) {
                bool answered = false;
                (void)moduleNames("all", &answered);
                EXPECT_TRUE(answered);

                (void)moduleNames("loaded", &answered);
                EXPECT_TRUE(answered);
            }
        });
    }

    for (auto& th : threads) th.join();
}

// -----------------------------------------------------------------------------
// Process all plugins, then each thread loads a disjoint slice. Tests the load
// path under concurrent pressure; needs logos_host (LOGOS_HOST_PATH), since
// every module must still be loaded after the calls that loaded them returned.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentLoadPlugin) {
    processAll(kModuleCount);
    ASSERT_EQ(countOf(knownModules(), modules), kModuleCount);

    Barrier barrier(kThreads);
    std::vector<std::thread> threads;

    int perThread = kModuleCount / kThreads;

    for (int t = 0; t < kThreads; ++t) {
        int start = t * perThread;
        int end = (t == kThreads - 1) ? kModuleCount : start + perThread;

        threads.emplace_back([&, start, end]() {
            barrier.wait();
            for (int i = start; i < end; ++i) {
                std::string name = modules[i].name.toStdString();
                (void)loadModule(name, "module_only");
            }
        });
    }

    for (auto& th : threads) th.join();

    const QStringList known = knownModules();
    EXPECT_EQ(countOf(known, modules), kModuleCount);
    for (const DummyModule& m : modules)
        EXPECT_TRUE(known.contains(m.name)) << m.name.toStdString();

    // Every host outlives the call that loaded it.
    EXPECT_EQ(loadedCountAfterSettling(modules), kModuleCount);
}

// -----------------------------------------------------------------------------
// All threads hammer the SAME small set of plugins with loads. Tests mutex
// contention and the "already loaded" fast-return branch.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentLoadSamePlugin) {
    constexpr int kSmall = kThreads;
    processAll(kSmall);
    ASSERT_EQ(countOf(knownModules(), modules), kSmall);

    Barrier barrier(kThreads);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            barrier.wait();
            for (int i = 0; i < kSmall; ++i) {
                std::string name = modules[i].name.toStdString();
                (void)loadModule(name, "module_only");
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(countOf(knownModules(), modules), kSmall);
    EXPECT_EQ(loadedCountAfterSettling(modules), kSmall);
}

// -----------------------------------------------------------------------------
// Each thread loads a disjoint slice with its required dependencies. Tests
// dependency resolution and loadMutex acquisition from multiple threads.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentLoadWithDeps) {
    processAll(kModuleCount);
    ASSERT_EQ(countOf(knownModules(), modules), kModuleCount);

    Barrier barrier(kThreads);
    std::vector<std::thread> threads;

    int perThread = kModuleCount / kThreads;

    for (int t = 0; t < kThreads; ++t) {
        int start = t * perThread;
        int end = (t == kThreads - 1) ? kModuleCount : start + perThread;

        threads.emplace_back([&, start, end]() {
            barrier.wait();
            for (int i = start; i < end; ++i) {
                std::string name = modules[i].name.toStdString();
                (void)loadModule(name, "required");
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(countOf(knownModules(), modules), kModuleCount);
    EXPECT_EQ(loadedCountAfterSettling(modules), kModuleCount);
}

// -----------------------------------------------------------------------------
// Half the threads load while the other half unload the same small module set.
// Tests the load/unload interplay under concurrent pressure.
// -----------------------------------------------------------------------------
TEST_F(RealPluginThreadSafetyTest, ConcurrentLoadUnloadInterleaved) {
    constexpr int kSmall = kThreads;
    processAll(kSmall);
    ASSERT_EQ(countOf(knownModules(), modules), kSmall);

    Barrier barrier(kThreads);
    std::vector<std::thread> threads;

    int loaders   = kThreads / 2;
    int unloaders = kThreads - loaders;

    for (int t = 0; t < loaders; ++t) {
        threads.emplace_back([&]() {
            barrier.wait();
            for (int iter = 0; iter < 10; ++iter) {
                for (int i = 0; i < kSmall; ++i) {
                    std::string name = modules[i].name.toStdString();
                    (void)loadModule(name, "module_only");
                }
            }
        });
    }

    for (int t = 0; t < unloaders; ++t) {
        threads.emplace_back([&]() {
            barrier.wait();
            for (int iter = 0; iter < 10; ++iter) {
                for (int i = 0; i < kSmall; ++i) {
                    std::string name = modules[i].name.toStdString();
                    (void)unloadModule(name);
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    // All modules must still be registered; load/unload must not corrupt the registry.
    const QStringList known = knownModules();
    EXPECT_EQ(countOf(known, modules), kSmall);
    for (int i = 0; i < kSmall; ++i)
        EXPECT_TRUE(known.contains(modules[i].name)) << modules[i].name.toStdString();
}
