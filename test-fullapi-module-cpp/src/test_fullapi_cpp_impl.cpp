#include "test_fullapi_cpp_impl.h"

#include <cstdio>
#include <cstdlib>

#include <logos_codec.h>   // logos::toJson<T> — SPIKE trace only, see below

// Every echo returns its input unchanged so a consumer can assert round-trip
// fidelity per type, and so the C++ and Rust providers answer identically (the
// cross-language parity check). `whoAmI()` is the one intentional difference.

namespace {

// SPIKE INSTRUMENTATION (env-gated, off by default).
//
// The question this spike answers is which VALUE crosses the boundary, and the
// only authoritative answer is the one the provider's own typed parameter
// holds — past QML's coercion, past the consumer wrapper, past the wire. A
// return value cannot answer it (a coerced value echoes back perfectly), and
// ModuleProxy deliberately never logs arguments, so the observation has to
// happen here.
//
// Rendered with the canonical codec rather than hand-formatted, so the trace
// shows exactly the canonical JSON of what arrived: a uint64 stays exact, bytes
// stay tagged.
bool traceOn()
{
    static const bool on = std::getenv("LOGOS_FULLAPI_TRACE") != nullptr;
    return on;
}

template <class T>
void trace(const char* method, const T& v)
{
    if (!traceOn()) return;
    std::fprintf(stderr, "FULLAPI_RECEIVED %s %s\n", method,
                 logos::toJson<T>(v).dump().c_str());
    std::fflush(stderr);
}

} // namespace

// ── Identity ─────────────────────────────────────────────────────────────────

std::string TestFullapiCppImpl::whoAmI() { return "test_fullapi_cpp"; }

// ── Scalar echoes ─────────────────────────────────────────────────────────────

std::string          TestFullapiCppImpl::echoString(const std::string& v)        { trace("echoString", v); return v; }
std::vector<uint8_t> TestFullapiCppImpl::echoBytes(const std::vector<uint8_t>& v) { trace("echoBytes", v); return v; }
int64_t              TestFullapiCppImpl::echoInt(int64_t v)                       { trace("echoInt", v); return v; }
uint64_t             TestFullapiCppImpl::echoUint(uint64_t v)                     { trace("echoUint", v); return v; }
double               TestFullapiCppImpl::echoDouble(double v)                     { trace("echoDouble", v); return v; }
bool                 TestFullapiCppImpl::echoBool(bool v)                         { trace("echoBool", v); return v; }
nlohmann::json       TestFullapiCppImpl::echoAny(const nlohmann::json& v)         { trace("echoAny", v); return v; }

// ── Container echoes ──────────────────────────────────────────────────────────

std::vector<std::string> TestFullapiCppImpl::echoStringList(const std::vector<std::string>& v) { trace("echoStringList", v); return v; }
std::vector<int64_t>     TestFullapiCppImpl::echoIntList(const std::vector<int64_t>& v)         { trace("echoIntList", v); return v; }
std::vector<uint64_t>    TestFullapiCppImpl::echoUintList(const std::vector<uint64_t>& v)       { trace("echoUintList", v); return v; }
std::vector<double>      TestFullapiCppImpl::echoDoubleList(const std::vector<double>& v)       { trace("echoDoubleList", v); return v; }
std::vector<bool>        TestFullapiCppImpl::echoBoolList(const std::vector<bool>& v)           { trace("echoBoolList", v); return v; }
LogosList                TestFullapiCppImpl::echoList(const LogosList& v)                       { trace("echoList", v); return v; }
LogosMap                 TestFullapiCppImpl::echoMap(const LogosMap& v)                         { trace("echoMap", v); return v; }

// ── Return-only types ─────────────────────────────────────────────────────────

void TestFullapiCppImpl::doVoid() {}

StdLogosResult TestFullapiCppImpl::makeResult(bool ok) {
    if (!ok) {
        return {false, {}, "deliberate error for testing"};
    }
    nlohmann::json data;
    data["provider"] = "test_fullapi_cpp";
    data["ok"]       = true;
    return {true, data, ""};
}

// ── Event trigger drivers ─────────────────────────────────────────────────────
// Each fires its typed event and returns true. The typed event method bodies
// live in the generated test_fullapi_cpp_events.cpp.

// ── Arity ────────────────────────────────────────────────────────────────────
//
// `i=<decimal>|s=<utf8>|b=<lowercase hex>` — one string that pins all three
// arguments and their order. A container return would confound "the arguments
// arrived in the right slots" with "the container encoding survived"; a digest
// does not.

std::string TestFullapiCppImpl::echoTriple(int64_t i, const std::string& s,
                                           const std::vector<uint8_t>& b)
{
    std::string hex;
    hex.reserve(b.size() * 2);
    static const char* kDigits = "0123456789abcdef";
    for (uint8_t byte : b) {
        hex.push_back(kDigits[byte >> 4]);
        hex.push_back(kDigits[byte & 0x0f]);
    }
    return "i=" + std::to_string(i) + "|s=" + s + "|b=" + hex;
}

bool TestFullapiCppImpl::fireStringEvent(const std::string& v)              { stringEvent(v);     return true; }
bool TestFullapiCppImpl::fireBytesEvent(const std::vector<uint8_t>& v)      { bytesEvent(v);      return true; }
bool TestFullapiCppImpl::fireIntEvent(int64_t v)                           { intEvent(v);        return true; }
bool TestFullapiCppImpl::fireUintEvent(uint64_t v)                         { uintEvent(v);       return true; }
bool TestFullapiCppImpl::fireDoubleEvent(double v)                         { doubleEvent(v);     return true; }
bool TestFullapiCppImpl::fireBoolEvent(bool v)                             { boolEvent(v);       return true; }
bool TestFullapiCppImpl::fireAnyEvent(const nlohmann::json& v)             { anyEvent(v);        return true; }
bool TestFullapiCppImpl::fireStringListEvent(const std::vector<std::string>& v) { stringListEvent(v); return true; }
bool TestFullapiCppImpl::fireIntListEvent(const std::vector<int64_t>& v)   { intListEvent(v);    return true; }
bool TestFullapiCppImpl::fireUintListEvent(const std::vector<uint64_t>& v) { uintListEvent(v);   return true; }
bool TestFullapiCppImpl::fireDoubleListEvent(const std::vector<double>& v) { doubleListEvent(v); return true; }
bool TestFullapiCppImpl::fireBoolListEvent(const std::vector<bool>& v)     { boolListEvent(v);   return true; }
bool TestFullapiCppImpl::fireListEvent(const LogosList& v)                 { listEvent(v);       return true; }
bool TestFullapiCppImpl::fireTripleEvent(int64_t i, const std::string& s, const std::vector<uint8_t>& b) { tripleEvent(i, s, b); return true; }
bool TestFullapiCppImpl::fireMapEvent(const LogosMap& v)                   { mapEvent(v);        return true; }
