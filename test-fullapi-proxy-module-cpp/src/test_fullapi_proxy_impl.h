#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_fullapi_proxy — universal C++ consumer/proxy.
//
// Consumes the full `full_api` surface of EITHER provider (test_fullapi_cpp or
// test_fullapi_rust) via an INTERFACE dependency (interfaces/full_api.h), and
// RE-EXPOSES the exact same surface itself — every call forwards to the bound
// provider, every provider event is captured and re-emitted. So the proxy is
// itself a drop-in `full_api` provider that delegates to another.
//
// useProvider(name) selects the runtime target (cross-language: bind to the C++
// or the Rust provider). getLastEvent() returns a short summary of the most
// recent forwarded event so a CLI doctest can assert event delivery.
//
// The bind/call sites live in the .cpp (which includes the generated
// logos_sdk.h); this header the generator parses stays free of codegen types.
// NO trailing `// comments` on declaration lines (the parser needs a trailing `;`).
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap, LogosList, nlohmann::json
#include <logos_module_context.h>  // LogosModuleContext base (gives modules())
#include <logos_result.h>          // StdLogosResult

class TestFullapiProxyImpl : public LogosModuleContext {
public:
    TestFullapiProxyImpl() = default;
    ~TestFullapiProxyImpl() = default;

    void onContextReady() override;

    // ── Control ──────────────────────────────────────────────────────────────
    // Bind the interface to `name` for method forwarding + subscribe to its
    // events. Returns true.
    bool useProvider(const std::string& name);
    // The module name currently bound.
    std::string currentProvider();
    // Short summary ("<eventName>:<payload-or-size>") of the most recently
    // forwarded event; empty until one arrives.
    std::string getLastEvent();
    // Round-trip every array type through the bound provider (LP path) and
    // report the received sizes as a string — a CLI-observable probe of the
    // provider's array decode, isolated from the Qt/ui-host transport.
    std::string probeArrays();

    // Forward a few methods to the bound provider through the ASYNC client
    // wrappers (<method>Async), storing each result as its completion fires.
    // probeAsync() kicks them off and returns immediately; getAsyncProbe()
    // reports what has arrived. A CLI drives the pair with the daemon event loop
    // in between (it delivers the completions), exercising the proxy → provider
    // ASYNC call path (the sync forwards above only cover the blocking path).
    std::string probeAsync();
    std::string getAsyncProbe();

    // ── Forwarded full_api surface (same signatures as IFullApi) ─────────────
    std::string          whoAmI();
    std::string          echoString(const std::string& v);
    std::vector<uint8_t> echoBytes(const std::vector<uint8_t>& v);
    int64_t              echoInt(int64_t v);
    uint64_t             echoUint(uint64_t v);
    double               echoDouble(double v);
    bool                 echoBool(bool v);
    nlohmann::json       echoAny(const nlohmann::json& v);
    std::vector<std::string> echoStringList(const std::vector<std::string>& v);
    std::vector<int64_t>     echoIntList(const std::vector<int64_t>& v);
    std::vector<uint64_t>    echoUintList(const std::vector<uint64_t>& v);
    std::vector<double>      echoDoubleList(const std::vector<double>& v);
    std::vector<bool>        echoBoolList(const std::vector<bool>& v);
    LogosList                echoList(const LogosList& v);
    LogosMap                 echoMap(const LogosMap& v);
    void           doVoid();
    std::string echoTriple(int64_t i, const std::string& s, const std::vector<uint8_t>& b);
    StdLogosResult makeResult(bool ok);
    bool fireStringEvent(const std::string& v);
    bool fireBytesEvent(const std::vector<uint8_t>& v);
    bool fireIntEvent(int64_t v);
    bool fireUintEvent(uint64_t v);
    bool fireDoubleEvent(double v);
    bool fireBoolEvent(bool v);
    bool fireAnyEvent(const nlohmann::json& v);
    bool fireStringListEvent(const std::vector<std::string>& v);
    bool fireIntListEvent(const std::vector<int64_t>& v);
    bool fireUintListEvent(const std::vector<uint64_t>& v);
    bool fireDoubleListEvent(const std::vector<double>& v);
    bool fireBoolListEvent(const std::vector<bool>& v);
    bool fireListEvent(const LogosList& v);
    bool fireMapEvent(const LogosMap& v);
    bool fireTripleEvent(int64_t i, const std::string& s, const std::vector<uint8_t>& b);

    // ── Re-emitted events (mirror full_api) ──────────────────────────────────
logos_events:
    void stringEvent(const std::string& v);
    void bytesEvent(const std::vector<uint8_t>& v);
    void intEvent(int64_t v);
    void uintEvent(uint64_t v);
    void doubleEvent(double v);
    void boolEvent(bool v);
    void anyEvent(const nlohmann::json& v);
    void stringListEvent(const std::vector<std::string>& v);
    void intListEvent(const std::vector<int64_t>& v);
    void uintListEvent(const std::vector<uint64_t>& v);
    void doubleListEvent(const std::vector<double>& v);
    void boolListEvent(const std::vector<bool>& v);
    void listEvent(const LogosList& v);
    void mapEvent(const LogosMap& v);
    void tripleEvent(int64_t i, const std::string& s, const std::vector<uint8_t>& b);

private:
    void subscribeToTarget();

    std::string m_target = "test_fullapi_cpp";
    std::string m_lastEvent;

    // Results collected by probeAsync()'s async completions. The lp async
    // callbacks fire on the transport's delivery thread, so guard the fields
    // with a mutex (probeAsync writes, getAsyncProbe reads from another call).
    std::mutex m_asyncMx;
    int m_asyncDone = 0;
    std::vector<std::string> m_asyncFails;
    std::string m_asyncWho;
};
