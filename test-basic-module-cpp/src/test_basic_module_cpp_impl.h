#pragma once

// Pure-C++ mirror of `test_basic_module`. Exercises every parameter/return
// type the code generator needs to translate between C++ std types and Qt
// on the wire:
//
//   void, bool, int64_t, uint64_t, double, std::string,
//   std::vector<std::string>, std::vector<uint8_t>,
//   LogosMap / LogosList (nlohmann::json aliases), StdLogosResult
//
// Absolutely no Qt headers here — `logos-cpp-generator --from-header`
// parses this file as text to derive method signatures, then emits a
// `test_basic_module_cpp_qt_glue.h` + `_dispatch.cpp` that does all the
// QVariant ↔ std conversion. See spec.md in logos-cpp-sdk/cpp-generator/docs/
// for the full conversion table.
//
// Event emission: declare events in the `logos_events:` section below
// (Qt-`signals:`-style). The generator parses each prototype, emits a
// `<name>_events.cpp` defining the body (which routes the typed args
// through LogosModuleContext::emitEventImpl_ → LogosProviderBase
// → QRO `eventResponse`), and a `<name>.lidl` sidecar so consumer-side
// codegen can expose typed `on<EventName>(callback)` accessors.

#include <cstdint>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap, LogosList (nlohmann::json aliases)
#include <logos_module_context.h>  // LogosModuleContext base; pulls in `logos_events`
#include <logos_result.h>          // StdLogosResult

class TestBasicModuleCppImpl : public LogosModuleContext {
public:
    TestBasicModuleCppImpl() = default;
    ~TestBasicModuleCppImpl() = default;

    // ── Return type: void ────────────────────────────────────────────────
    void doNothing();
    void doNothingWithArgs(const std::string& a, int64_t b);

    // ── Return type: bool ────────────────────────────────────────────────
    bool returnTrue();
    bool returnFalse();
    bool isPositive(int64_t value);

    // ── Return type: int64_t ─────────────────────────────────────────────
    int64_t returnInt();
    int64_t addInts(int64_t a, int64_t b);
    int64_t stringLength(const std::string& s);

    // ── Return type: uint64_t ────────────────────────────────────────────
    uint64_t returnUint();
    uint64_t echoUint(uint64_t n);

    // ── Return type: double ──────────────────────────────────────────────
    double returnDouble();
    double addDoubles(double a, double b);

    // ── Return type: std::string ─────────────────────────────────────────
    std::string returnString();
    std::string echo(const std::string& input);
    std::string concat(const std::string& a, const std::string& b);

    // ── Return type: StdLogosResult ──────────────────────────────────────
    // Generator emits a StdLogosResult → Qt LogosResult conversion in glue,
    // so over the wire this comes out the same shape as test_basic_module's
    // `LogosResult` methods: { success, value, error }.
    StdLogosResult successResult();
    StdLogosResult errorResult();
    StdLogosResult resultWithMap();
    StdLogosResult resultWithList();
    StdLogosResult validateInput(const std::string& input);

    // ── Return type: LogosMap (json object) ─────────────────────────────
    // `jsonReturn=true` → glue calls nlohmannToQVariant to unpack into
    // QVariantMap, which then serialises as a regular JSON object over RPC.
    LogosMap returnMap();
    LogosMap makeMap(const std::string& key, const std::string& value);

    // ── Return type: LogosList (json array) ─────────────────────────────
    LogosList returnList();
    LogosList makeList(const std::string& a, const std::string& b);

    // ── Return type: std::vector<std::string> ───────────────────────────
    std::vector<std::string> returnStringList();
    std::vector<std::string> splitString(const std::string& input);

    // ── Return type: std::vector<uint8_t> (bytes) ───────────────────────
    std::vector<uint8_t> returnBytes();
    int64_t byteArraySize(const std::vector<uint8_t>& data);

    // ── Parameter types ─────────────────────────────────────────────────
    int64_t echoInt(int64_t n);
    bool echoBool(bool b);
    std::string joinStrings(const std::vector<std::string>& list);

    // ── Argument counts 0–5 (same matrix as the Qt module) ──────────────
    std::string noArgs();
    std::string oneArg(const std::string& a);
    std::string twoArgs(const std::string& a, int64_t b);
    std::string threeArgs(const std::string& a, int64_t b, bool c);
    std::string fourArgs(const std::string& a, int64_t b, bool c, const std::string& d);
    std::string fiveArgs(const std::string& a, int64_t b, bool c, const std::string& d, int64_t e);

    // ── Events ──────────────────────────────────────────────────────────
    // Driver methods called from logoscore — they fire the typed events
    // declared in `logos_events:` below. Mirrors test_basic_module's
    // Qt-side `emitTestEvent` / `emitMultiArgEvent` Q_INVOKABLE pair so
    // the integration tests can use the same test_event_system.cpp
    // harness against both modules.
    void emitTestEvent(const std::string& data);
    void emitMultiArgEvent(const std::string& name, int64_t count);

    // Bool-returning variants of the emit drivers. Same semantic, but
    // safe to chain via `logoscore -c` (the void-returning twins above
    // can't be — logoscore needs a value to format as `Result:`).
    // Used by the context-cpp round-trip tests, which chain
    //   subscribe → trigger → read
    // in a single logoscore invocation.
    bool triggerTestEvent(const std::string& data);
    bool triggerMultiArgEvent(const std::string& name, int64_t count);

    // Typed events. Codegen emits bodies in test_basic_module_cpp_events.cpp
    // and exposes typed `onTestEvent(...)` / `onMultiArgEvent(...)`
    // accessors on the consumer-side wrapper.
logos_events:
    void testEvent(const std::string& data);
    void multiArgEvent(const std::string& name, int64_t count);
};
