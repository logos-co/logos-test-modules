#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_ipc_new_api_module — universal C++ consumer.
//
// Exercises inter-module communication from a `interface: "universal"` module:
// every call below reaches `test_basic_module` / `test_extlib_module` through
// the generated type-safe wrappers (`modules().<dep>.<method>(...)`), which
// talk the logos-protocol C ABI (lp_*) directly. There is no Qt in this
// module's own translation units and no hand-written plugin loader — the
// generator derives the LIDL contract from this header and emits the glue.
//
// It previously used the `LOGOS_PROVIDER` / `LOGOS_METHOD` provider-header
// path plus raw `LogosAPIClient::invokeRemoteMethod` calls. Both are gone:
// the dynamic invoke is now a host-class privilege, and typed wrappers are
// the ordinary way for a module to consume another.
//
// The method surface is deliberately UNCHANGED across that migration (same
// names, same arity, same wire types) so the ipc test group keeps asserting
// exactly what it asserted before.
//
// NO trailing `// comments` on declaration lines (the parser needs a `;`).
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <string>

#include <logos_json.h>            // LogosMap / LogosList (nlohmann::json aliases)
#include <logos_module_context.h>  // LogosModuleContext base; gives modules()
#include <logos_result.h>          // StdLogosResult

class TestIpcNewApiImpl : public LogosModuleContext {
public:
    TestIpcNewApiImpl() = default;
    ~TestIpcNewApiImpl() = default;

    // ── Calls to test_basic_module ───────────────────────────────────────────
    std::string callBasicEcho(const std::string& input);
    int64_t callBasicAddInts(int64_t a, int64_t b);
    bool callBasicReturnTrue();
    std::string callBasicNoArgs();
    std::string callBasicFiveArgs(const std::string& a, int64_t b, bool c, const std::string& d, int64_t e);
    StdLogosResult callBasicSuccessResult();
    StdLogosResult callBasicErrorResult();
    std::string callBasicResultMapField(const std::string& key);

    // ── Calls to test_extlib_module ──────────────────────────────────────────
    std::string callExtlibReverse(const std::string& input);
    std::string callExtlibUppercase(const std::string& input);
    int64_t callExtlibCountChars(const std::string& input);

    // ── Cross-module chaining ────────────────────────────────────────────────
    std::string chainEchoThenReverse(const std::string& input);
    std::string chainUppercaseThenConcat(const std::string& a, const std::string& b);

    // ── Typed wrappers, kept as distinct methods ─────────────────────────────
    // These used to be the only call sites going through the generated
    // wrappers, as opposed to raw invokeRemoteMethod. Now every method above
    // goes through them too, so these are simple duplicates — retained
    // because the ipc test group calls them by name.
    std::string wrapperBasicEcho(const std::string& input);
    std::string wrapperExtlibReverse(const std::string& input);

    // ── Events ───────────────────────────────────────────────────────────────
    // Asks test_basic_module to emit its own event, then emits our own.
    void triggerBasicEvent(const std::string& data);

logos_events:
    // Emitted by triggerBasicEvent, after the downstream call returns.
    // Previously emitted dynamically by name; declaring it here is what makes
    // it typed and discoverable by consumer-side codegen.
    void triggeredBasicEvent(const std::string& data);
};
