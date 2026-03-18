#ifndef LOGOS_TEST_H
#define LOGOS_TEST_H

/**
 * @file logos_test.h
 * @brief Lightweight unit-test framework for Logos module tests.
 *
 * No external dependencies.  Tests self-register at static-initialisation
 * time via the LOGOS_TEST macro, so just #include this header and define
 * your tests — the test runner in main.cpp discovers them automatically.
 *
 * Usage:
 *
 *   #include "logos_test.h"
 *   #include "logos_mock.h"
 *
 *   LOGOS_TEST(my_feature_works) {
 *       LogosMockSetup mock;
 *       mock.when("mod", "method").thenReturn(QVariant(7));
 *       // ...
 *       LOGOS_ASSERT_EQ(someValue, 7);
 *   }
 */

#include <QCoreApplication>
#include <QString>
#include <QVariant>
#include <QList>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>

// Allow QString and QVariant to be streamed into std::ostream (for assertion messages)
inline std::ostream& operator<<(std::ostream& os, const QString& s)  { return os << s.toStdString(); }
inline std::ostream& operator<<(std::ostream& os, const QVariant& v) { return os << v.toString().toStdString(); }

// ── Test failure exception ───────────────────────────────────────────────────

class LogosTestFailure : public std::runtime_error {
public:
    explicit LogosTestFailure(const std::string& msg) : std::runtime_error(msg) {}
};

// ── Assertion macros ─────────────────────────────────────────────────────────

#define LOGOS_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT failed: " #expr \
                 << "  (" << __FILE__ << ":" << __LINE__ << ")"; \
            throw LogosTestFailure(_oss.str()); \
        } \
    } while (false)

#define LOGOS_ASSERT_TRUE(expr)  LOGOS_ASSERT(expr)
#define LOGOS_ASSERT_FALSE(expr) LOGOS_ASSERT(!(expr))

#define LOGOS_ASSERT_EQ(actual, expected) \
    do { \
        auto _a = (actual); \
        auto _e = (expected); \
        if (!(_a == _e)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_EQ failed: expected [" \
                 << _e << "] but got [" << _a << "]" \
                 << "  (" << __FILE__ << ":" << __LINE__ << ")"; \
            throw LogosTestFailure(_oss.str()); \
        } \
    } while (false)

#define LOGOS_ASSERT_NE(actual, expected) \
    do { \
        auto _a = (actual); \
        auto _e = (expected); \
        if (_a == _e) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_NE failed: both equal [" << _a << "]" \
                 << "  (" << __FILE__ << ":" << __LINE__ << ")"; \
            throw LogosTestFailure(_oss.str()); \
        } \
    } while (false)

// ── Test runner ──────────────────────────────────────────────────────────────

class LogosTestRunner {
public:
    struct TestEntry {
        QString name;
        std::function<void()> fn;
    };

    static LogosTestRunner& instance() {
        static LogosTestRunner runner;
        return runner;
    }

    bool registerTest(const char* name, std::function<void()> fn) {
        m_tests.append({QString::fromUtf8(name), fn});
        return true;
    }

    /**
     * @brief Run all registered tests.
     * @return 0 if all pass, 1 if any fail.
     */
    int runAll() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n=== Logos Module Unit Tests (New Provider API) ===\n\n";

        for (const TestEntry& t : m_tests) {
            std::cout << "  [ RUN  ] " << t.name.toStdString() << "\n";
            try {
                t.fn();
                std::cout << "  [ PASS ] " << t.name.toStdString() << "\n";
                ++passed;
            } catch (const LogosTestFailure& ex) {
                std::cout << "  [ FAIL ] " << t.name.toStdString()
                          << "\n           " << ex.what() << "\n";
                ++failed;
            } catch (const std::exception& ex) {
                std::cout << "  [ FAIL ] " << t.name.toStdString()
                          << "\n           Unexpected exception: " << ex.what() << "\n";
                ++failed;
            } catch (...) {
                std::cout << "  [ FAIL ] " << t.name.toStdString()
                          << "\n           Unknown exception\n";
                ++failed;
            }
        }

        std::cout << "\n=== Results: " << passed << " passed, "
                  << failed << " failed";
        if (m_tests.isEmpty()) {
            std::cout << " (no tests registered!)";
        }
        std::cout << " ===\n\n";

        return failed > 0 ? 1 : 0;
    }

private:
    QList<TestEntry> m_tests;
};

// ── LOGOS_TEST macro ─────────────────────────────────────────────────────────

#define LOGOS_TEST(name) \
    static void _logos_test_fn_##name(); \
    static bool _logos_test_reg_##name = \
        LogosTestRunner::instance().registerTest(#name, _logos_test_fn_##name); \
    static void _logos_test_fn_##name()

// ── Test main ────────────────────────────────────────────────────────────────

#define LOGOS_TEST_MAIN() \
    int main(int argc, char* argv[]) { \
        QCoreApplication app(argc, argv); \
        return LogosTestRunner::instance().runAll(); \
    }

#endif // LOGOS_TEST_H
