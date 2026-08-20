#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_fullapi_qtproxy — the QT-TYPED consumer of the full_api contract.
//
// The conformance matrix has two providers. Its consumer axis needs a point
// where the value is decoded by the QT-typed generated wrapper, and neither
// existing proxy is one: `test_fullapi_proxy` is `interface: universal` with the
// default lp consumer surface, and `test_fullapi_proxy_rust` is the Rust client.
// Both bypass the Qt wrappers entirely, which is why known.json records that
// they cannot substitute for this module.
//
// WHAT SELECTS THE QT CONSUMER NOW. Two independent keys in metadata.json:
//
//   "interface": "universal"                 — the PROVIDER surface. This module
//       is a header-first cdylib: the LIDL contract below is derived from THIS
//       header, the C exports come from `logos_module_impl.h`, and the uniform
//       Qt plugin glue is generated. That is why every declaration here is
//       std-typed and Qt-free.
//   "codegen": { "consumer_api_style": "qt" } — the CONSUMER surface. The
//       generated `modules().bind_full_api(name)` hands back a QT-TYPED
//       `FullApi`: QString / QByteArray / qlonglong / qulonglong / QVariantList
//       / QVariantMap / LogosResult. Without this key a cdylib-packaged module
//       defaults to `lp` and this module would measure the same path
//       test_fullapi_proxy already covers.
//
// The two used to be one decision — "no `interface` key" selected apiStyle=qt
// for the provider AND the consumer — which is the shape this module was
// written in, and the reason it was the last caller of the generator's
// `--provider-header` mode. That mode is gone; the axis it conflated is now
// declared, so the same consumer surface is reached from a supported shape.
//
// WHAT ONLY THIS PATH REACHES, unchanged by the migration:
//
//   * registry entry M3 — a one-key `_bytes` map reinterpreted as bytes happens
//     in logos-protocol's `nlohmannToQVariant`, which is on the way to a Qt
//     consumer and nowhere else.
//   * the ASYNC return path. `useCallMode` routes every forwarded call through
//     the sync or the async wrapper, so the whole case table replays twice.
//
// The forwarding surface mirrors test-fullapi-proxy-module-cpp 1:1 (33 methods,
// 15 events) so the matrix replays cases.json through this consumer unchanged:
//   logoscore call test_fullapi_qtproxy echoInt 42
//
// EVENTS have no sync/async axis: a subscription is a callback either way (the
// generated `onXxxEvent` accessors are the only form), so `useCallMode` governs
// METHODS only. An event cell measured through this proxy is the same cell in
// both modes; that is a property of the generated surface, not an omission.
//
// Authoring rules (universal / Qt-free header — the generator parses this file
// as TEXT):
//   * no Qt headers and no Qt types here. Every Qt type in this module lives in
//     the .cpp, on the CONSUMER side of the boundary.
//   * `any` -> a bare `nlohmann::json`; `[any]` -> LogosList; `{tstr:any}` ->
//     LogosMap.
//   * event params that are non-scalar MUST be `const T&`; scalars by value.
//   * NO trailing `// comments` on declaration lines: the parser only accepts a
//     line ending in `;`, so a trailing comment silently drops the declaration.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap, LogosList, nlohmann::json
#include <logos_module_context.h>  // LogosModuleContext base (gives modules())
#include <logos_result.h>          // StdLogosResult

class TestFullapiQtproxyImpl : public LogosModuleContext {
public:
    TestFullapiQtproxyImpl() = default;
    ~TestFullapiQtproxyImpl() = default;

    void onContextReady() override;

    // ── Control ─────────────────────────────────────────────────────────────
    //
    // `///` and not `//`: a doc comment becomes the method's DESCRIPTION in the
    // derived contract and therefore in the plugin's QMetaObject, which is what
    // `lm methods --json` prints. The pre-migration header carried these through
    // `--provider-header`; the universal header parser reads the same marker, so
    // they survive the move rather than silently disappearing from the surface.
    /// Bind the full_api interface to `moduleName` and subscribe to its events.
    bool useProvider(const std::string& moduleName);
    /// The module name currently bound.
    std::string currentProvider();
    /// Route every forwarded METHOD through the generated sync or async wrapper.
    bool useCallMode(const std::string& mode);
    /// Which token store this image reads, and whether it holds the auth tokens.
    std::string tokenProbe();
    /// makeResult(true) rendered by the Qt wrapper and by this module's own surface.
    std::string resultShapeProbe();
    /// "sync" or "async".
    std::string currentCallMode();
    /// "ok-sync" / "ok-async" (which generated table actually ran), or a failure.
    std::string lastCallStatus();
    /// "<eventName>:<payload-or-size>" for the most recently forwarded event.
    std::string getLastEvent();
    /// Round-trip every array type and report the received sizes (shape only).
    std::string probeArrays();
    /// Render the discriminating returns through the SYNC wrappers.
    std::string syncProbe();
    /// Fire the same set through the ASYNC wrappers; returns immediately.
    std::string probeAsync();
    /// The async renderings once the completions have landed (else "pending=N").
    std::string getAsyncProbe();

    // ── Forwarded full_api surface ──────────────────────────────────────────
    std::string              whoAmI();
    std::string              echoString(const std::string& v);
    std::vector<uint8_t>     echoBytes(const std::vector<uint8_t>& v);
    int64_t                  echoInt(int64_t v);
    uint64_t                 echoUint(uint64_t v);
    double                   echoDouble(double v);
    bool                     echoBool(bool v);
    nlohmann::json           echoAny(const nlohmann::json& v);
    std::vector<std::string> echoStringList(const std::vector<std::string>& v);
    std::vector<int64_t>     echoIntList(const std::vector<int64_t>& v);
    std::vector<uint64_t>    echoUintList(const std::vector<uint64_t>& v);
    std::vector<double>      echoDoubleList(const std::vector<double>& v);
    std::vector<bool>        echoBoolList(const std::vector<bool>& v);
    LogosList                echoList(const LogosList& v);
    LogosMap                 echoMap(const LogosMap& v);
    void                     doVoid();
    std::string              echoTriple(int64_t i, const std::string& s, const std::vector<uint8_t>& b);
    StdLogosResult           makeResult(bool ok);
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

    // ── Re-emitted events (mirror full_api) ─────────────────────────────────
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
    void recordAsync(const std::string& key, const std::string& rendered);

    std::string m_provider = "test_fullapi_cpp";
    // sync is the default so an existing caller (and every cell measured before
    // the call-mode switch existed) keeps the surface it had.
    bool m_async = false;
    std::string m_lastCallStatus = "ok-sync";
    std::string m_lastEvent;

    // There is no unsubscribe on the wrapper, and re-binding must not stack
    // callbacks: subscribing twice to the same provider made every event arrive
    // twice (measured), which would double every event-position cell in the
    // matrix. Subscribe at most once per provider name, and have each callback
    // drop the delivery if its provider is no longer the bound one.
    std::set<std::string> m_subscribed;

    // probeAsync() completions land on whichever thread the transport delivers
    // on; getAsyncProbe() reads them from a separate call.
    std::mutex m_asyncMx;
    std::map<std::string, std::string> m_asyncResults;
    int m_asyncDone = 0;
};
