#include "test_basic_module_impl.h"

#include <chrono>
#include <sstream>
#include <thread>

// Every body below is a straight transcription of the Qt plugin's, with the
// Qt containers replaced by their std / nlohmann counterparts. The RESULTS are
// deliberately byte-identical strings — the integration suite asserts on them
// literally ("Result: fiveArgs(a, 1, true, b, 2)"), so any drift in the
// formatting would be a real regression, not cosmetics.

namespace {

// QStringLiteral("twoArgs(%1, %2)").arg(a).arg(b) has no std equivalent, and
// std::ostringstream turns bools into "1"/"0" rather than "true"/"false".
// One helper keeps the bool spelling in a single place.
inline const char* boolText(bool v) { return v ? "true" : "false"; }

}  // namespace

// ── Return type: void ────────────────────────────────────────────────────────

void TestBasicModuleImpl::doNothing()
{
}

void TestBasicModuleImpl::doNothingWithArgs(const std::string& a, int64_t b)
{
    (void)a;
    (void)b;
}

// ── Return type: bool ────────────────────────────────────────────────────────

bool TestBasicModuleImpl::returnTrue()
{
    return true;
}

bool TestBasicModuleImpl::returnFalse()
{
    return false;
}

bool TestBasicModuleImpl::isPositive(int64_t value)
{
    return value > 0;
}

// ── Return type: int ─────────────────────────────────────────────────────────

int64_t TestBasicModuleImpl::returnInt()
{
    return 42;
}

int64_t TestBasicModuleImpl::addInts(int64_t a, int64_t b)
{
    return a + b;
}

int64_t TestBasicModuleImpl::stringLength(const std::string& s)
{
    return static_cast<int64_t>(s.size());
}

// ── Return type: string ──────────────────────────────────────────────────────

std::string TestBasicModuleImpl::returnString()
{
    return "test_basic_module";
}

std::string TestBasicModuleImpl::echo(const std::string& input)
{
    return input;
}

std::string TestBasicModuleImpl::concat(const std::string& a, const std::string& b)
{
    return a + b;
}

// ── Return type: result ──────────────────────────────────────────────────────
//
// StdLogosResult is {bool success; nlohmann::json value; std::string error;}.
// The glue converts it to the same Qt LogosResult {success, value, error}
// shape the Qt plugin returned, so the wire payload is unchanged.

StdLogosResult TestBasicModuleImpl::successResult()
{
    return {true, "operation succeeded", ""};
}

StdLogosResult TestBasicModuleImpl::errorResult()
{
    return {false, nlohmann::json(), "deliberate error for testing"};
}

StdLogosResult TestBasicModuleImpl::resultWithMap()
{
    nlohmann::json map;
    map["name"] = "test";
    map["count"] = 42;
    map["active"] = true;
    return {true, map, ""};
}

StdLogosResult TestBasicModuleImpl::resultWithList()
{
    nlohmann::json item1;
    item1["id"] = 1;
    item1["label"] = "first";
    nlohmann::json item2;
    item2["id"] = 2;
    item2["label"] = "second";
    return {true, nlohmann::json::array({item1, item2}), ""};
}

StdLogosResult TestBasicModuleImpl::validateInput(const std::string& input)
{
    if (input.empty()) {
        return {false, nlohmann::json(), "input cannot be empty"};
    }
    nlohmann::json data;
    data["input"] = input;
    data["length"] = static_cast<int64_t>(input.size());
    return {true, data, ""};
}

// ── Return type: any (QVariant) ──────────────────────────────────────────────

nlohmann::json TestBasicModuleImpl::returnVariantInt()
{
    return 99;
}

nlohmann::json TestBasicModuleImpl::returnVariantString()
{
    return "variant_string";
}

nlohmann::json TestBasicModuleImpl::returnVariantMap()
{
    nlohmann::json map;
    map["key"] = "value";
    map["number"] = 7;
    return map;
}

nlohmann::json TestBasicModuleImpl::returnVariantList()
{
    return nlohmann::json::array({"alpha", "beta", "gamma"});
}

// ── Return type: JSON array ──────────────────────────────────────────────────

LogosList TestBasicModuleImpl::returnJsonArray()
{
    return nlohmann::json::array({1, 2, 3});
}

LogosList TestBasicModuleImpl::makeJsonArray(const std::string& a, const std::string& b)
{
    return nlohmann::json::array({a, b});
}

// ── Return type: string list ─────────────────────────────────────────────────

std::vector<std::string> TestBasicModuleImpl::returnStringList()
{
    return {"one", "two", "three"};
}

std::vector<std::string> TestBasicModuleImpl::splitString(const std::string& input)
{
    // QString::split(",", Qt::SkipEmptyParts) — empty fields are dropped, and
    // an input with no comma yields the whole string as the single element.
    std::vector<std::string> out;
    std::string field;
    std::istringstream stream(input);
    while (std::getline(stream, field, ',')) {
        if (!field.empty()) out.push_back(field);
    }
    return out;
}

// ── Parameter types ──────────────────────────────────────────────────────────

int64_t TestBasicModuleImpl::echoInt(int64_t n)
{
    return n;
}

bool TestBasicModuleImpl::echoBool(bool b)
{
    return b;
}

std::string TestBasicModuleImpl::joinStrings(const std::vector<std::string>& list)
{
    std::string out;
    for (size_t i = 0; i < list.size(); ++i) {
        if (i) out += ", ";
        out += list[i];
    }
    return out;
}

int64_t TestBasicModuleImpl::byteArraySize(const std::vector<uint8_t>& data)
{
    return static_cast<int64_t>(data.size());
}

std::string TestBasicModuleImpl::urlToString(const std::string& url)
{
    // The Qt version took a QUrl and called url.toString(); QUrl has no
    // universal spelling, so the parameter is now the string form the caller
    // was serialising to anyway. Round-tripping through a URL parser here
    // would CHANGE the observable answer (QUrl normalises), so it does not.
    return url;
}

// ── Argument counts 0–5 ──────────────────────────────────────────────────────

std::string TestBasicModuleImpl::noArgs()
{
    return "noArgs()";
}

std::string TestBasicModuleImpl::oneArg(const std::string& a)
{
    return "oneArg(" + a + ")";
}

std::string TestBasicModuleImpl::twoArgs(const std::string& a, int64_t b)
{
    return "twoArgs(" + a + ", " + std::to_string(b) + ")";
}

std::string TestBasicModuleImpl::threeArgs(const std::string& a, int64_t b, bool c)
{
    return "threeArgs(" + a + ", " + std::to_string(b) + ", " + boolText(c) + ")";
}

std::string TestBasicModuleImpl::fourArgs(const std::string& a, int64_t b, bool c,
                                          const std::string& d)
{
    return "fourArgs(" + a + ", " + std::to_string(b) + ", " + boolText(c) + ", " + d + ")";
}

std::string TestBasicModuleImpl::fiveArgs(const std::string& a, int64_t b, bool c,
                                          const std::string& d, int64_t e)
{
    return "fiveArgs(" + a + ", " + std::to_string(b) + ", " + boolText(c) + ", " + d + ", "
           + std::to_string(e) + ")";
}

// ── Events ───────────────────────────────────────────────────────────────────
//
// The Qt plugin emitted these dynamically:
//     emit eventResponse("testEvent", QVariantList() << data);
// The typed emitters below carry the SAME event names and the same payload
// arity; their bodies are generated into test_basic_module_events.cpp.

void TestBasicModuleImpl::emitTestEvent(const std::string& data)
{
    testEvent(data);
}

void TestBasicModuleImpl::emitMultiArgEvent(const std::string& name, int64_t count)
{
    multiArgEvent(name, count);
}

// ── Async helpers ────────────────────────────────────────────────────────────

std::string TestBasicModuleImpl::echoWithDelay(const std::string& value, int64_t delayMs)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    return value;
}

// ── Crash testing ────────────────────────────────────────────────────────────
//
// Used by the crash-isolation integration test in logos-logoscore-cli to prove
// that a faulty module brings down only its host subprocess, not the logoscore
// daemon. A null deref produces a real SIGSEGV under any sanitizer or debugger,
// which is exactly what we want to defend against.

void TestBasicModuleImpl::crashOnDemand()
{
    // Force the optimizer to actually emit the deref. `volatile` defeats
    // -O2 dead-store elimination; reading back guarantees the load fires.
    volatile int* p = nullptr;
    *p = 0xDEAD;
    (void)*p;
}
