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

// ── UTF-8 ────────────────────────────────────────────────────────────────────
// Every std::string crossing this module's contract is UTF-8, and "length"
// means CHARACTERS (Unicode code points). In UTF-8 a character is one lead
// byte plus zero or more continuation bytes (top bits 10), so counting the
// bytes that are NOT continuations counts the characters — for any character,
// of any width, including the 4-byte ones above the BMP.
inline bool isContinuationByte(char c)
{
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

int64_t characterCount(const std::string& s)
{
    int64_t n = 0;
    for (char c : s) {
        if (!isContinuationByte(c)) ++n;
    }
    return n;
}

// ── URL ──────────────────────────────────────────────────────────────────────
// Deterministic ASCII lowercase. std::tolower is locale-dependent and would
// also touch bytes >= 0x80, which in UTF-8 are fragments of characters.
inline char asciiLower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

inline bool isSchemeStart(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool isSchemeChar(char c)
{
    return isSchemeStart(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
}

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
    // Characters, not bytes: "héllo" is 5 even though it is 6 bytes of UTF-8.
    // An emoji outside the BMP is 1 — the byte count says 4 and Qt's UTF-16
    // QString said 2, and neither of those is a number of characters.
    return characterCount(s);
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
    // Same definition of length as stringLength: characters, not bytes.
    data["length"] = characterCount(input);
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
    // The Qt version took a QUrl and returned url.toString(); QUrl has no
    // universal spelling, so the parameter is the string form the caller was
    // serialising to anyway. What the method OWES its caller is unchanged:
    // the URL back, with the parts that are defined to be case-insensitive
    // normalised to lower case. That is the scheme and the host, and nothing
    // else — the path and the query are case-sensitive, so "/a/../b?x=1" is
    // returned exactly as given (collapsing the "/.." would change which
    // resource the URL names, and no caller asked for that).
    std::string out = url;

    // ── scheme ──────────────────────────────────────────────────────────────
    // scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) followed by ':'.
    size_t colon = std::string::npos;
    if (!out.empty() && isSchemeStart(out[0])) {
        size_t i = 1;
        while (i < out.size() && isSchemeChar(out[i])) ++i;
        if (i < out.size() && out[i] == ':') colon = i;
    }
    if (colon == std::string::npos) {
        // No scheme: nothing here is case-insensitive, so nothing is touched.
        return out;
    }
    for (size_t i = 0; i < colon; ++i) out[i] = asciiLower(out[i]);

    // ── host ────────────────────────────────────────────────────────────────
    // Only an authority-based URL ("scheme://…") has a host. It runs to the
    // first '/', '?' or '#'.
    if (out.compare(colon + 1, 2, "//") != 0) return out;
    const size_t authority = colon + 3;
    size_t authorityEnd = authority;
    while (authorityEnd < out.size() && out[authorityEnd] != '/' && out[authorityEnd] != '?'
           && out[authorityEnd] != '#') {
        ++authorityEnd;
    }

    // Userinfo ("user:password@") is case-SENSITIVE and stays as it is; the
    // host (and any ":port", which is digits) is what gets lowercased.
    size_t host = authority;
    for (size_t i = authority; i < authorityEnd; ++i) {
        if (out[i] == '@') host = i + 1;
    }
    for (size_t i = host; i < authorityEnd; ++i) out[i] = asciiLower(out[i]);

    return out;
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
