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
// Event emission: the generator detects the `std::function emitEvent`
// member by name and wires it to LogosProviderBase::emitEvent in the
// generated glue layer, so the impl can fire events without touching Qt.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <logos_json.h>     // LogosMap, LogosList (nlohmann::json aliases)
#include <logos_result.h>   // StdLogosResult

class TestBasicModuleCppImpl {
public:
    TestBasicModuleCppImpl() = default;
    ~TestBasicModuleCppImpl() = default;

    // Generated glue wires this in its ctor — call it to emit events. The
    // two-string signature (name, json-serialised data) is the one the
    // parser recognises; the generator emits a shim that dispatches to
    // LogosProviderBase::emitEvent with a QVariantList payload.
    std::function<void(const std::string& eventName, const std::string& data)> emitEvent;

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
    void emitTestEvent(const std::string& data);
    void emitMultiArgEvent(const std::string& name, int64_t count);
};
