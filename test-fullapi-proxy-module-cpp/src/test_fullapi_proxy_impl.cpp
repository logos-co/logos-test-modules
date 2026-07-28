#include "test_fullapi_proxy_impl.h"

#include <string>
#include <cmath>

// Generated at build time: LogosModules with the name-baked dependency wrappers
// AND (because metadata.json lists interface_dependencies) a
// bind_full_api(moduleName) factory returning the bound `FullApi` wrapper.
#include "logos_sdk.h"

// The lp consumer wrapper collapses a few types relative to the faithful
// full_api contract this proxy re-exposes:
//   * uint             -> int64_t          (scalar cast)
//   * [int]/[uint]/[float64]/[bool] -> LogosList (JSON array)
//   * any / {tstr:any} -> LogosMap         (both are nlohmann::json, so no-op)
// We convert at the forwarding boundary so the proxy's own surface stays the
// exact full_api shape (a consumer can bind `full_api` to this proxy too).

// ── Lifecycle + control ──────────────────────────────────────────────────────

void TestFullapiProxyImpl::onContextReady() {
    subscribeToTarget();
}

bool TestFullapiProxyImpl::useProvider(const std::string& name) {
    m_target = name;
    subscribeToTarget();
    return true;
}

std::string TestFullapiProxyImpl::currentProvider() { return m_target; }
std::string TestFullapiProxyImpl::getLastEvent()     { return m_lastEvent; }

std::string TestFullapiProxyImpl::probeArrays() {
    auto p = modules().bind_full_api(m_target);
    const auto il = p.echoIntList(LogosList({1, 2, 3}));
    const auto ul = p.echoUintList(LogosList({1, 2}));
    const auto dl = p.echoDoubleList(LogosList({1.5, 2.5}));
    const auto bl = p.echoBoolList(LogosList({true, false}));
    const std::vector<std::string> sIn{"a", "b"};
    const auto sl = p.echoStringList(sIn);
    const auto al = p.echoList(LogosList({1, 2, 3}));
    return "intList=" + std::to_string(il.size()) +
           " uintList=" + std::to_string(ul.size()) +
           " doubleList=" + std::to_string(dl.size()) +
           " boolList=" + std::to_string(bl.size()) +
           " stringList=" + std::to_string(sl.size()) +
           " anyList=" + std::to_string(al.size());
}

// Forward the FULL full_api surface to the bound provider through the generated
// std ASYNC client wrappers (`<method>Async(args, callback)`) and VERIFY each
// result. Every completion lands on the lp transport's delivery thread, so the
// tally is guarded by a mutex. probeAsync() kicks all of them off and returns
// "started"; getAsyncProbe() (a separate CLI call, after the daemon has pumped
// the deliveries) reports a single token "ALL_OK_ASYNC:<provider>" or
// "FAIL_ASYNC:<methods>" — the async, lp-path mirror of the UI's full-surface
// runMethodsAsync token, exercised against both the C++ and the Rust provider.
static const int kProxyAsyncExpected = 16;

std::string TestFullapiProxyImpl::probeAsync() {
    {
        std::lock_guard<std::mutex> lk(m_asyncMx);
        m_asyncDone = 0;
        m_asyncFails.clear();
        m_asyncWho.clear();
    }
    auto p = modules().bind_full_api(m_target);
    auto tick = [this](bool ok, const char* name) {
        std::lock_guard<std::mutex> lk(m_asyncMx);
        if (!ok) m_asyncFails.emplace_back(name);
        ++m_asyncDone;
    };
    p.whoAmIAsync([this](std::string w) {
        std::lock_guard<std::mutex> lk(m_asyncMx);
        m_asyncWho = w;
        if (w.empty()) m_asyncFails.emplace_back("whoAmI");
        ++m_asyncDone;
    });
    p.echoStringAsync(std::string("z"), [tick](std::string r) { tick(r == "z", "echoString"); });
    p.echoIntAsync(7, [tick](int64_t r) { tick(r == 7, "echoInt"); });
    p.echoUintAsync(7, [tick](int64_t r) { tick(r == 7, "echoUint"); });
    p.echoDoubleAsync(2.5, [tick](double r) { tick(std::fabs(r - 2.5) <= 1e-9, "echoDouble"); });
    p.echoBoolAsync(true, [tick](bool r) { tick(r, "echoBool"); });
    p.echoBytesAsync(std::vector<uint8_t>{1, 2, 3}, [tick](std::vector<uint8_t> r) { tick(r.size() == 3, "echoBytes"); });
    p.echoAnyAsync(nlohmann::json("x"), [tick](nlohmann::json r) { tick(r == nlohmann::json("x"), "echoAny"); });
    p.echoStringListAsync(std::vector<std::string>{"a", "b"}, [tick](std::vector<std::string> r) { tick(r.size() == 2, "echoStringList"); });
    p.echoMapAsync(LogosMap{{"k", "v"}}, [tick](LogosMap r) { tick(r.size() == 1, "echoMap"); });
    p.makeResultAsync(true, [tick](StdLogosResult r) { tick(r.success, "makeResult"); });
    p.echoIntListAsync(LogosList({1, 2, 3}), [tick](LogosList r) { tick(r.size() == 3, "echoIntList"); });
    p.echoUintListAsync(LogosList({1, 2}), [tick](LogosList r) { tick(r.size() == 2, "echoUintList"); });
    p.echoDoubleListAsync(LogosList({1.5, 2.5}), [tick](LogosList r) { tick(r.size() == 2, "echoDoubleList"); });
    p.echoBoolListAsync(LogosList({true, false}), [tick](LogosList r) { tick(r.size() == 2, "echoBoolList"); });
    p.echoListAsync(LogosList({1, 2, 3}), [tick](LogosList r) { tick(r.size() == 3, "echoList"); });
    return "started";
}

std::string TestFullapiProxyImpl::getAsyncProbe() {
    std::lock_guard<std::mutex> lk(m_asyncMx);
    if (m_asyncDone < kProxyAsyncExpected)
        return "pending=" + std::to_string(m_asyncDone);
    if (m_asyncFails.empty())
        return "ALL_OK_ASYNC:" + m_asyncWho;
    std::string joined;
    for (size_t i = 0; i < m_asyncFails.size(); ++i) {
        if (i) joined += ",";
        joined += m_asyncFails[i];
    }
    return "FAIL_ASYNC:" + joined;
}

// ── Forwarded methods ────────────────────────────────────────────────────────

std::string TestFullapiProxyImpl::whoAmI() {
    return modules().bind_full_api(m_target).whoAmI();
}
std::string TestFullapiProxyImpl::echoString(const std::string& v) {
    return modules().bind_full_api(m_target).echoString(v);
}
std::vector<uint8_t> TestFullapiProxyImpl::echoBytes(const std::vector<uint8_t>& v) {
    return modules().bind_full_api(m_target).echoBytes(v);
}
int64_t TestFullapiProxyImpl::echoInt(int64_t v) {
    return modules().bind_full_api(m_target).echoInt(v);
}
uint64_t TestFullapiProxyImpl::echoUint(uint64_t v) {
    return static_cast<uint64_t>(
        modules().bind_full_api(m_target).echoUint(static_cast<int64_t>(v)));
}
double TestFullapiProxyImpl::echoDouble(double v) {
    return modules().bind_full_api(m_target).echoDouble(v);
}
bool TestFullapiProxyImpl::echoBool(bool v) {
    return modules().bind_full_api(m_target).echoBool(v);
}
nlohmann::json TestFullapiProxyImpl::echoAny(const nlohmann::json& v) {
    return modules().bind_full_api(m_target).echoAny(v);
}
std::vector<std::string> TestFullapiProxyImpl::echoStringList(const std::vector<std::string>& v) {
    return modules().bind_full_api(m_target).echoStringList(v);
}
std::vector<int64_t> TestFullapiProxyImpl::echoIntList(const std::vector<int64_t>& v) {
    LogosList res = modules().bind_full_api(m_target).echoIntList(LogosList(v));
    return res.get<std::vector<int64_t>>();
}
std::vector<uint64_t> TestFullapiProxyImpl::echoUintList(const std::vector<uint64_t>& v) {
    LogosList res = modules().bind_full_api(m_target).echoUintList(LogosList(v));
    return res.get<std::vector<uint64_t>>();
}
std::vector<double> TestFullapiProxyImpl::echoDoubleList(const std::vector<double>& v) {
    LogosList res = modules().bind_full_api(m_target).echoDoubleList(LogosList(v));
    return res.get<std::vector<double>>();
}
std::vector<bool> TestFullapiProxyImpl::echoBoolList(const std::vector<bool>& v) {
    LogosList res = modules().bind_full_api(m_target).echoBoolList(LogosList(v));
    return res.get<std::vector<bool>>();
}
LogosList TestFullapiProxyImpl::echoList(const LogosList& v) {
    return modules().bind_full_api(m_target).echoList(v);
}
LogosMap TestFullapiProxyImpl::echoMap(const LogosMap& v) {
    return modules().bind_full_api(m_target).echoMap(v);
}
void TestFullapiProxyImpl::doVoid() {
    modules().bind_full_api(m_target).doVoid();
}
std::string TestFullapiProxyImpl::echoTriple(int64_t i, const std::string& s,
                                             const std::vector<uint8_t>& b) {
    return modules().bind_full_api(m_target).echoTriple(i, s, b);
}
StdLogosResult TestFullapiProxyImpl::makeResult(bool ok) {
    return modules().bind_full_api(m_target).makeResult(ok);
}

// ── Forwarded event trigger drivers ──────────────────────────────────────────
// Each forwards to the bound provider, whose emission our subscription (below)
// captures and re-emits.

bool TestFullapiProxyImpl::fireStringEvent(const std::string& v) {
    return modules().bind_full_api(m_target).fireStringEvent(v);
}
bool TestFullapiProxyImpl::fireBytesEvent(const std::vector<uint8_t>& v) {
    return modules().bind_full_api(m_target).fireBytesEvent(v);
}
bool TestFullapiProxyImpl::fireIntEvent(int64_t v) {
    return modules().bind_full_api(m_target).fireIntEvent(v);
}
bool TestFullapiProxyImpl::fireUintEvent(uint64_t v) {
    return modules().bind_full_api(m_target).fireUintEvent(static_cast<int64_t>(v));
}
bool TestFullapiProxyImpl::fireDoubleEvent(double v) {
    return modules().bind_full_api(m_target).fireDoubleEvent(v);
}
bool TestFullapiProxyImpl::fireBoolEvent(bool v) {
    return modules().bind_full_api(m_target).fireBoolEvent(v);
}
bool TestFullapiProxyImpl::fireAnyEvent(const nlohmann::json& v) {
    return modules().bind_full_api(m_target).fireAnyEvent(v);
}
bool TestFullapiProxyImpl::fireStringListEvent(const std::vector<std::string>& v) {
    return modules().bind_full_api(m_target).fireStringListEvent(v);
}
bool TestFullapiProxyImpl::fireIntListEvent(const std::vector<int64_t>& v) {
    return modules().bind_full_api(m_target).fireIntListEvent(LogosList(v));
}
bool TestFullapiProxyImpl::fireUintListEvent(const std::vector<uint64_t>& v) {
    return modules().bind_full_api(m_target).fireUintListEvent(LogosList(v));
}
bool TestFullapiProxyImpl::fireDoubleListEvent(const std::vector<double>& v) {
    return modules().bind_full_api(m_target).fireDoubleListEvent(LogosList(v));
}
bool TestFullapiProxyImpl::fireBoolListEvent(const std::vector<bool>& v) {
    return modules().bind_full_api(m_target).fireBoolListEvent(LogosList(v));
}
bool TestFullapiProxyImpl::fireListEvent(const LogosList& v) {
    return modules().bind_full_api(m_target).fireListEvent(v);
}
bool TestFullapiProxyImpl::fireTripleEvent(int64_t i, const std::string& s,
                                           const std::vector<uint8_t>& b) {
    return modules().bind_full_api(m_target).fireTripleEvent(i, s, b);
}
bool TestFullapiProxyImpl::fireMapEvent(const LogosMap& v) {
    return modules().bind_full_api(m_target).fireMapEvent(v);
}

// ── Subscribe to the bound provider's events: capture a summary + re-emit ─────

void TestFullapiProxyImpl::subscribeToTarget() {
    auto api = modules().bind_full_api(m_target);
    api.onStringEvent([this](const std::string& v) {
        m_lastEvent = "stringEvent:" + v; stringEvent(v);
    });
    api.onBytesEvent([this](const std::vector<uint8_t>& v) {
        m_lastEvent = "bytesEvent:size=" + std::to_string(v.size()); bytesEvent(v);
    });
    api.onIntEvent([this](int64_t v) {
        m_lastEvent = "intEvent:" + std::to_string(v); intEvent(v);
    });
    api.onUintEvent([this](int64_t v) {
        m_lastEvent = "uintEvent:" + std::to_string(v); uintEvent(static_cast<uint64_t>(v));
    });
    api.onDoubleEvent([this](double v) {
        m_lastEvent = "doubleEvent:" + std::to_string(v); doubleEvent(v);
    });
    api.onBoolEvent([this](bool v) {
        m_lastEvent = std::string("boolEvent:") + (v ? "true" : "false"); boolEvent(v);
    });
    api.onAnyEvent([this](const LogosMap& v) {
        m_lastEvent = "anyEvent:" + v.dump(); anyEvent(v);
    });
    api.onStringListEvent([this](const std::vector<std::string>& v) {
        m_lastEvent = "stringListEvent:size=" + std::to_string(v.size()); stringListEvent(v);
    });
    api.onIntListEvent([this](const LogosList& v) {
        m_lastEvent = "intListEvent:size=" + std::to_string(v.size());
        intListEvent(v.get<std::vector<int64_t>>());
    });
    api.onUintListEvent([this](const LogosList& v) {
        m_lastEvent = "uintListEvent:size=" + std::to_string(v.size());
        uintListEvent(v.get<std::vector<uint64_t>>());
    });
    api.onDoubleListEvent([this](const LogosList& v) {
        m_lastEvent = "doubleListEvent:size=" + std::to_string(v.size());
        doubleListEvent(v.get<std::vector<double>>());
    });
    api.onBoolListEvent([this](const LogosList& v) {
        m_lastEvent = "boolListEvent:size=" + std::to_string(v.size());
        boolListEvent(v.get<std::vector<bool>>());
    });
    api.onListEvent([this](const LogosList& v) {
        m_lastEvent = "listEvent:size=" + std::to_string(v.size()); listEvent(v);
    });
    api.onMapEvent([this](const LogosMap& v) {
        m_lastEvent = "mapEvent:size=" + std::to_string(v.size()); mapEvent(v);
    });
    // The only MULTI-parameter event: the summary carries all three arguments
    // so a subscriber that mixes up positional slots is visible here too, not
    // just in the re-emit.
    api.onTripleEvent([this](int64_t i, const std::string& str, const std::vector<uint8_t>& b) {
        m_lastEvent = "tripleEvent:i=" + std::to_string(i) + ",s=" + str
                    + ",b=size" + std::to_string(b.size());
        tripleEvent(i, str, b);
    });
}
