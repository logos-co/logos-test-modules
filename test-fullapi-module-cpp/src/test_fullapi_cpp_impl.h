#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_fullapi_cpp — universal C++ provider covering the ENTIRE supported type
// surface.
//
// It exercises, for the code generator + wire, every type that works end-to-end
// across C++ std, the cdylib/Rust ABI, and the Qt/UI boundary:
//
//   method params/returns : tstr, bstr, int, uint, float64, bool, any,
//                           [tstr], [int], [uint], [float64], [bool],
//                           [any] (LogosList), {tstr:any} (LogosMap),
//                           result + void (returns only)
//   event params          : the same set minus result/void
//
// The Rust provider `test_fullapi_rust` implements the SAME method + event names
// and shapes, so a consumer can bind the `full_api` interface to either one.
//
// Authoring rules (universal / Qt-free):
//   * no Qt headers — the impl is parsed as text by the generator, which maps
//     std / LogosMap / LogosList / StdLogosResult to the wire and emits the Qt
//     glue in generated_code/.
//   * `any` -> a bare `nlohmann::json` (NOT LogosMap/LogosList, which the parser
//     recognises by name as map/list; anything else falls back to `any`).
//   * event params that are non-scalar (string, vector, LogosMap/List) MUST be
//     `const T&`; scalars are by value.
//   * NO trailing `// comments` on declaration lines: the header parser only
//     accepts a line that ends with `;`, so a trailing comment silently drops
//     the method/event. Type annotations therefore live above each group.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap, LogosList, nlohmann::json
#include <logos_module_context.h>  // LogosModuleContext base + `logos_events`
#include <logos_result.h>          // StdLogosResult

class TestFullapiCppImpl : public LogosModuleContext {
public:
    TestFullapiCppImpl() = default;
    ~TestFullapiCppImpl() = default;

    // Identity — lets a consumer prove which provider a bound interface resolved
    // to ("test_fullapi_cpp" vs "test_fullapi_rust").
    std::string whoAmI();

    // ── Scalar echoes: param type == return type ─────────────────────────────
    // tstr / bstr / int / uint / float64 / bool / any
    std::string          echoString(const std::string& v);
    std::vector<uint8_t> echoBytes(const std::vector<uint8_t>& v);
    int64_t              echoInt(int64_t v);
    uint64_t             echoUint(uint64_t v);
    double               echoDouble(double v);
    bool                 echoBool(bool v);
    nlohmann::json       echoAny(const nlohmann::json& v);

    // ── Container echoes ─────────────────────────────────────────────────────
    // [tstr] / [int] / [uint] / [float64] / [bool] / [any] / {tstr:any}
    std::vector<std::string> echoStringList(const std::vector<std::string>& v);
    std::vector<int64_t>     echoIntList(const std::vector<int64_t>& v);
    std::vector<uint64_t>    echoUintList(const std::vector<uint64_t>& v);
    std::vector<double>      echoDoubleList(const std::vector<double>& v);
    std::vector<bool>        echoBoolList(const std::vector<bool>& v);
    LogosList                echoList(const LogosList& v);
    LogosMap                 echoMap(const LogosMap& v);

    // ── Arity ────────────────────────────────────────────────────────────────
    // The contract's only MULTI-parameter surfaces. Every other method takes
    // zero or one argument, so argument POSITION — and a bstr sitting anywhere
    // but first — had no coverage at all. The return is a digest of all three
    // so ONE comparison pins each value AND its slot: swap two arguments and
    // the digest changes.
    std::string echoTriple(int64_t i, const std::string& s, const std::vector<uint8_t>& b);

    // ── Return-only types ────────────────────────────────────────────────────
    // void return (no value); result return (success or error)
    void           doVoid();
    StdLogosResult makeResult(bool ok);

    // ── Event trigger drivers ────────────────────────────────────────────────
    // Each fires the correspondingly-typed event declared in `logos_events:`.
    // Bool-returning (not void) so `logoscore call` can invoke and read them —
    // void returns make the CLI exit non-zero. A consumer subscribes to the
    // event and reads back the captured payload.
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

    // ── Typed events (one per event-legal type) ──────────────────────────────
    // Codegen emits bodies in test_fullapi_cpp_events.cpp and exposes typed
    // on<Event>(callback) accessors on the consumer-side wrapper. Order mirrors
    // the trigger drivers above:
    //   tstr / bstr / int / uint / float64 / bool / any /
    //   [tstr] / [int] / [uint] / [float64] / [bool] / [any] / {tstr:any}
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
};
