#include "test_basic_module_cpp_impl.h"

#include <cstddef>   // std::size_t (used in splitString below)
#include <utility>   // std::move (used in splitString below)

// Keep return shapes closely aligned with test-basic-module's Qt versions
// so the docker smoke / integration tests can exercise the same matrix
// against both modules. Some values intentionally differ where they
// identify the concrete module implementation — notably `returnString()`
// reports `"test_basic_module_cpp"` so the test can distinguish which of
// the two modules answered the call.

// ── void ─────────────────────────────────────────────────────────────────

void TestBasicModuleCppImpl::doNothing() {}
void TestBasicModuleCppImpl::doNothingWithArgs(const std::string&, int64_t) {}

// ── bool ─────────────────────────────────────────────────────────────────

bool TestBasicModuleCppImpl::returnTrue()  { return true;  }
bool TestBasicModuleCppImpl::returnFalse() { return false; }
bool TestBasicModuleCppImpl::isPositive(int64_t value) { return value > 0; }

// ── int64_t ──────────────────────────────────────────────────────────────

int64_t TestBasicModuleCppImpl::returnInt() { return 42; }
int64_t TestBasicModuleCppImpl::addInts(int64_t a, int64_t b) { return a + b; }
int64_t TestBasicModuleCppImpl::stringLength(const std::string& s) {
    return static_cast<int64_t>(s.size());
}

// ── uint64_t ─────────────────────────────────────────────────────────────

uint64_t TestBasicModuleCppImpl::returnUint() { return 99u; }
uint64_t TestBasicModuleCppImpl::echoUint(uint64_t n) { return n; }

// ── double ───────────────────────────────────────────────────────────────

double TestBasicModuleCppImpl::returnDouble() { return 3.5; }
double TestBasicModuleCppImpl::addDoubles(double a, double b) { return a + b; }

// ── std::string ──────────────────────────────────────────────────────────

std::string TestBasicModuleCppImpl::returnString() { return "test_basic_module_cpp"; }
std::string TestBasicModuleCppImpl::echo(const std::string& input) { return input; }
std::string TestBasicModuleCppImpl::concat(const std::string& a, const std::string& b) {
    return a + b;
}

// ── StdLogosResult ───────────────────────────────────────────────────────
// Mirrors test_basic_module's `successResult` / `errorResult` / `…WithMap`
// / `…WithList` / `validateInput` — payloads tuned so the pytest assertion
// matrix can compare row-for-row with the Qt-module expectations.

StdLogosResult TestBasicModuleCppImpl::successResult() {
    return {true, "operation succeeded", ""};
}

StdLogosResult TestBasicModuleCppImpl::errorResult() {
    return {false, {}, "deliberate error for testing"};
}

StdLogosResult TestBasicModuleCppImpl::resultWithMap() {
    nlohmann::json m;
    m["name"]   = "test";
    m["count"]  = 42;
    m["active"] = true;
    return {true, m, ""};
}

StdLogosResult TestBasicModuleCppImpl::resultWithList() {
    nlohmann::json list = nlohmann::json::array();
    list.push_back({{"id", 1}, {"label", "first"}});
    list.push_back({{"id", 2}, {"label", "second"}});
    return {true, list, ""};
}

StdLogosResult TestBasicModuleCppImpl::validateInput(const std::string& input) {
    if (input.empty()) {
        return {false, {}, "input cannot be empty"};
    }
    nlohmann::json data;
    data["input"]  = input;
    data["length"] = static_cast<int64_t>(input.size());
    return {true, data, ""};
}

// ── LogosMap ─────────────────────────────────────────────────────────────

LogosMap TestBasicModuleCppImpl::returnMap() {
    LogosMap m;
    m["key"]    = "value";
    m["number"] = 7;
    return m;
}

LogosMap TestBasicModuleCppImpl::makeMap(const std::string& key,
                                         const std::string& value) {
    LogosMap m;
    m[key] = value;
    return m;
}

// ── LogosList ────────────────────────────────────────────────────────────

LogosList TestBasicModuleCppImpl::returnList() {
    LogosList list = nlohmann::json::array();
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    return list;
}

LogosList TestBasicModuleCppImpl::makeList(const std::string& a,
                                            const std::string& b) {
    LogosList list = nlohmann::json::array();
    list.push_back(a);
    list.push_back(b);
    return list;
}

// ── std::vector<std::string> ─────────────────────────────────────────────

std::vector<std::string> TestBasicModuleCppImpl::returnStringList() {
    return {"one", "two", "three"};
}

std::vector<std::string> TestBasicModuleCppImpl::splitString(const std::string& input) {
    // Split on ','. Simple, matches the Qt module's semantics.
    std::vector<std::string> out;
    std::string cur;
    for (char c : input) {
        if (c == ',') { out.push_back(std::move(cur)); cur.clear(); }
        else          { cur.push_back(c); }
    }
    out.push_back(std::move(cur));
    return out;
}

// ── std::vector<uint8_t> ─────────────────────────────────────────────────

std::vector<uint8_t> TestBasicModuleCppImpl::returnBytes() {
    return {0x01, 0x02, 0x03, 0x04, 0x05};
}

int64_t TestBasicModuleCppImpl::byteArraySize(const std::vector<uint8_t>& data) {
    return static_cast<int64_t>(data.size());
}

// ── Parameter types ──────────────────────────────────────────────────────

int64_t TestBasicModuleCppImpl::echoInt(int64_t n)  { return n; }
bool    TestBasicModuleCppImpl::echoBool(bool b)    { return b; }

std::string TestBasicModuleCppImpl::joinStrings(const std::vector<std::string>& list) {
    std::string out;
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (i > 0) out += ",";
        out += list[i];
    }
    return out;
}

// ── 0–5 args ─────────────────────────────────────────────────────────────
// The format-string trick (QString("...arg() / arg()")) the Qt module uses
// doesn't translate directly; plain string concatenation produces the same
// output, which keeps the pytest expected-value strings identical.

static std::string toDec(int64_t n) { return std::to_string(n); }
static std::string toBoolStr(bool b) { return b ? "true" : "false"; }

std::string TestBasicModuleCppImpl::noArgs() {
    return "noArgs()";
}
std::string TestBasicModuleCppImpl::oneArg(const std::string& a) {
    return "oneArg(" + a + ")";
}
std::string TestBasicModuleCppImpl::twoArgs(const std::string& a, int64_t b) {
    return "twoArgs(" + a + ", " + toDec(b) + ")";
}
std::string TestBasicModuleCppImpl::threeArgs(const std::string& a, int64_t b, bool c) {
    return "threeArgs(" + a + ", " + toDec(b) + ", " + toBoolStr(c) + ")";
}
std::string TestBasicModuleCppImpl::fourArgs(const std::string& a, int64_t b,
                                              bool c, const std::string& d) {
    return "fourArgs(" + a + ", " + toDec(b) + ", " + toBoolStr(c) + ", " + d + ")";
}
std::string TestBasicModuleCppImpl::fiveArgs(const std::string& a, int64_t b,
                                              bool c, const std::string& d, int64_t e) {
    return "fiveArgs(" + a + ", " + toDec(b) + ", " + toBoolStr(c) + ", " + d + ", " + toDec(e) + ")";
}

// ── Events ───────────────────────────────────────────────────────────────

void TestBasicModuleCppImpl::emitTestEvent(const std::string& data) {
    if (emitEvent) emitEvent("testEvent", data);
}

void TestBasicModuleCppImpl::emitMultiArgEvent(const std::string& name, int64_t count) {
    // The generator-wired emitEvent takes (name, data). Pack the multi-arg
    // payload as a JSON string so the wire shape is lossless (Python test
    // just asserts both fragments appear in the stringified event).
    if (emitEvent) {
        nlohmann::json payload;
        payload["name"]  = name;
        payload["count"] = count;
        emitEvent("multiArgEvent", payload.dump());
    }
}
