#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_fullapi_ext_qtproxy — the QT-TYPED consumer of the full_api_ext contract.
//
// WHY IT EXISTS. The ext table ran 44 cases x 2 providers x ONE consumer (py).
// Its own comment said so. Everything the lossless Qt mapping widened lives in
// THIS contract and nowhere in full_api:
//
//   records                Blob / Opt / Wrapper -> generated nested structs
//   ?tstr                  -> std::optional<QString>            (was QVariant)
//   [Blob] / [Opt]         -> QList<Blob> / QList<Opt>
//   [[int]]                -> QList<QList<qlonglong>>
//   [bstr]                 -> QList<QByteArray>
//   {tstr: Blob}           -> QMap<QString, Blob>
//   {tstr: [bstr]}         -> QMap<QString, QList<QByteArray>>   (nested)
//
// None of those spellings had a consumer measuring them end to end. This module
// is that measurement: it is the SAME two-key shape as test_fullapi_qtproxy —
//
//   "interface": "universal"                 — the PROVIDER surface. A
//       header-first cdylib: the LIDL contract (records included) is derived
//       from THIS header, the C exports come from `logos_module_impl.h`, and
//       the uniform Qt plugin glue is generated. Every declaration here is
//       therefore std-typed and Qt-free.
//   "codegen": { "consumer_api_style": "qt" } — the CONSUMER surface. The
//       generated `modules().bind_full_api_ext(name)` hands back a QT-TYPED
//       `FullApiExt`, whose record types are NESTED (FullApiExt::Blob, …) and
//       so do not collide with the std-typed `Blob` / `Wrapper` / `Opt` this
//       header declares at global scope for the provider side.
//
// ONE DIVERGENCE IS BAKED INTO THE INTERFACE and must be read before a cell is
// blamed on the hop: `echoOptional`. interfaces/full_api_ext.lidl declares
// `-> result`, following test_fullapi_ext_rust.lidl — the file the matrix
// driver is given as --contract. test_fullapi_ext_cpp is header-first and still
// derives `-> ?tstr`. That is known-ext.json's
// `ext-optional-return-changed-on-one-provider`, not something this module
// introduced; what this module adds is a second surface on which it is visible,
// because the Qt wrapper decodes a `result` and one provider does not send one.
//
// EVENTS have no sync/async axis: a subscription is a callback either way, so
// `useCallMode` governs METHODS only. The single ext event (blobEvent) measures
// the same cell in both modes; that is a property of the generated surface, not
// an omission.
//
// Authoring rules (universal / Qt-free header — the generator parses this file
// as TEXT):
//   * no Qt headers and no Qt types here. Every Qt type in this module lives in
//     the .cpp, on the CONSUMER side of the boundary.
//   * a `struct` at file scope becomes a `type` decl in the derived contract;
//     the field spellings must match the interface .lidl exactly.
//   * event params that are non-scalar MUST be `const T&`; scalars by value.
//   * NO trailing `// comments` on declaration lines: the parser only accepts a
//     line ending in `;`, so a trailing comment silently drops the declaration.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap, LogosList, nlohmann::json
#include <logos_module_context.h>  // LogosModuleContext base (gives modules())
#include <logos_result.h>          // StdLogosResult

// The three records, declared exactly as both ext providers declare them.
struct Blob {
    std::string          id;
    uint64_t             n;
    std::vector<uint8_t> payload;
};

struct Wrapper {
    Blob                     inner;
    std::vector<std::string> tags;
    std::vector<Blob>        blobs;
};

struct Opt {
    std::string                         required;
    std::optional<std::string>          maybe;
    std::optional<uint64_t>             count;
    std::optional<std::vector<uint8_t>> blob;
};

class TestFullapiExtQtproxyImpl : public LogosModuleContext {
public:
    TestFullapiExtQtproxyImpl() = default;
    ~TestFullapiExtQtproxyImpl() = default;

    void onContextReady() override;

    // ── Control ─────────────────────────────────────────────────────────────
    //
    // `///` and not `//`: a doc comment becomes the method's DESCRIPTION in the
    // derived contract and therefore in the plugin's QMetaObject, which is what
    // `lm methods --json` prints.
    /// Bind the full_api_ext interface to `moduleName` and subscribe to its events.
    bool useProvider(const std::string& moduleName);
    /// The module name currently bound.
    std::string currentProvider();
    /// Route every forwarded METHOD through the generated sync or async wrapper.
    bool useCallMode(const std::string& mode);
    /// "sync" or "async".
    std::string currentCallMode();
    /// "ok-sync" / "ok-async" (which generated table actually ran), or a failure.
    std::string lastCallStatus();
    /// "<eventName>:<summary>" for the most recently forwarded event.
    std::string getLastEvent();
    /// Which token store this image reads, and whether it holds the auth tokens.
    std::string tokenProbe();
    /// Render every discriminating return through the SYNC wrappers.
    std::string syncProbe();
    /// Fire the same set through the ASYNC wrappers; returns immediately.
    std::string probeAsync();
    /// The async renderings once the completions have landed (else "pending=N").
    std::string getAsyncProbe();

    // ── Forwarded full_api_ext surface ──────────────────────────────────────
    std::string whoAmI();
    Blob    echoBlob(const Blob& v);
    Wrapper echoWrapper(const Wrapper& v);
    std::vector<Blob> echoBlobList(const std::vector<Blob>& v);
    std::map<std::string, Blob> echoBlobMap(const std::map<std::string, Blob>& v);
    std::vector<std::vector<uint8_t>> echoBytesList(const std::vector<std::vector<uint8_t>>& v);
    std::map<std::string, std::vector<uint8_t>> echoBytesMap(const std::map<std::string, std::vector<uint8_t>>& v);
    std::map<std::string, int64_t> echoIntMap(const std::map<std::string, int64_t>& v);
    std::map<std::string, std::string> echoStringMap(const std::map<std::string, std::string>& v);
    std::vector<std::vector<int64_t>> echoNestedInts(const std::vector<std::vector<int64_t>>& v);
    std::map<std::string, std::vector<std::vector<uint8_t>>> echoMapOfBytesLists(const std::map<std::string, std::vector<std::vector<uint8_t>>>& v);
    Opt echoOpt(const Opt& v);
    std::vector<Opt> echoOptList(const std::vector<Opt>& v);
    StdLogosResult echoOptional(const std::optional<std::string>& v);
    bool fireBlobEvent(const Blob& v);

    // ── Re-emitted events (mirror full_api_ext) ─────────────────────────────
logos_events:
    void blobEvent(const Blob& v);

private:
    void subscribeToTarget();
    void recordAsync(const std::string& key, const std::string& rendered);

    std::string m_provider = "test_fullapi_ext_cpp";
    // sync is the default so a caller that never sets a mode keeps the surface
    // the direct `py` consumer measures.
    bool m_async = false;
    std::string m_lastCallStatus = "ok-sync";
    std::string m_lastEvent;

    // There is no unsubscribe on the wrapper, and re-binding must not stack
    // callbacks: subscribing twice to the same provider makes every event
    // arrive twice, which would double every event-position cell. Subscribe at
    // most once per provider name, and have each callback drop the delivery if
    // its provider is no longer the bound one.
    std::set<std::string> m_subscribed;

    // probeAsync() completions land on whichever thread the transport delivers
    // on; getAsyncProbe() reads them from a separate call.
    std::mutex m_asyncMx;
    std::map<std::string, std::string> m_asyncResults;
    int m_asyncDone = 0;
};
