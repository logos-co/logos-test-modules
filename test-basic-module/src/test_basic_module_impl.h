#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_basic_module — universal C++ provider fixture.
//
// The point of this module is COVERAGE: it declares one method per
// return type, per parameter type and per argument count (0–5) the SDK has to
// carry over the wire, so a codec regression shows up here before it shows up
// in a real module. That matrix is preserved 1:1 across the move to
// `interface: "universal"`; this plain C++ class is now the contract, and the
// generator derives the LIDL from it and emits the Qt glue, the dispatch and
// the C-ABI exports.
//
// ── Type choices that are NOT arbitrary ─────────────────────────────────────
//   * `nlohmann::json` (NOT LogosMap / LogosList) for the returnVariant*
//     family. The parser maps the bare alias-expanded spelling to LIDL `any`,
//     which is `QVariant` on the Qt surface — exactly what those four methods
//     returned before. LogosMap/LogosList would have narrowed them to
//     QVariantMap/QVariantList.
//   * `LogosList` for the JSON-array pair: `[any]` → QVariantList. The old
//     declared type, QJsonArray, has no universal spelling. The VALUE on the
//     wire is the same JSON array; only the declared Qt type moves. The whole
//     set of types that moved is the table under "Universal migration — the
//     Qt types that moved" in this repo's README.
//   * `std::vector<uint8_t>` → bstr → QByteArray, so byteArraySize keeps its
//     exact parameter type.
//   * `std::vector<std::string>` → [tstr] → QStringList, so joinStrings /
//     returnStringList / splitString keep theirs.
//
// ── Strings ─────────────────────────────────────────────────────────────────
// Every `std::string` here is UTF-8, and every "length" this module reports is
// a number of CHARACTERS (Unicode code points) — see stringLength and
// validateInput. byteArraySize is the deliberate contrast: it takes a bstr and
// answers in bytes, so "héllo" is 5 to the first two and 6 to it.
//
// NO trailing `// comments` on declaration lines (the parser needs a `;`).
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <string>
#include <vector>

#include <logos_json.h>            // LogosMap / LogosList (nlohmann::json aliases)
#include <logos_module_context.h>  // LogosModuleContext base; pulls in `logos_events`
#include <logos_result.h>          // StdLogosResult

class TestBasicModuleImpl : public LogosModuleContext {
public:
    TestBasicModuleImpl() = default;
    ~TestBasicModuleImpl() = default;

    // ── Return type: void ────────────────────────────────────────────────────
    void doNothing();
    void doNothingWithArgs(const std::string& a, int64_t b);

    // ── Return type: bool ────────────────────────────────────────────────────
    bool returnTrue();
    bool returnFalse();
    bool isPositive(int64_t value);

    // ── Return type: int ─────────────────────────────────────────────────────
    int64_t returnInt();
    int64_t addInts(int64_t a, int64_t b);

    // Length of `s` in CHARACTERS (Unicode code points), the string being
    // UTF-8: stringLength("hello") == 5 and stringLength("héllo") == 5 too.
    // An emoji outside the BMP is one character and counts 1 — the byte count
    // says 4 and Qt's UTF-16 QString said 2, neither of which is a length of
    // any text.
    int64_t stringLength(const std::string& s);

    // ── Return type: string ──────────────────────────────────────────────────
    std::string returnString();
    std::string echo(const std::string& input);
    std::string concat(const std::string& a, const std::string& b);

    // ── Return type: result ──────────────────────────────────────────────────
    StdLogosResult successResult();
    StdLogosResult errorResult();
    StdLogosResult resultWithMap();
    StdLogosResult resultWithList();

    // {success, {input, length}} for a non-empty input, an error otherwise.
    // `length` is the same quantity stringLength returns — CHARACTERS.
    StdLogosResult validateInput(const std::string& input);

    // ── Return type: any (QVariant) ──────────────────────────────────────────
    nlohmann::json returnVariantInt();
    nlohmann::json returnVariantString();
    nlohmann::json returnVariantMap();
    nlohmann::json returnVariantList();

    // ── Return type: JSON array ──────────────────────────────────────────────
    LogosList returnJsonArray();
    LogosList makeJsonArray(const std::string& a, const std::string& b);

    // ── Return type: string list ─────────────────────────────────────────────
    std::vector<std::string> returnStringList();
    std::vector<std::string> splitString(const std::string& input);

    // ── Parameter types ──────────────────────────────────────────────────────
    int64_t echoInt(int64_t n);
    bool echoBool(bool b);
    std::string joinStrings(const std::vector<std::string>& list);
    int64_t byteArraySize(const std::vector<uint8_t>& data);

    // Normalise the case-insensitive parts of a URL and return it as a string:
    // the scheme and the host are lowercased, everything else is left exactly
    // as given (the path and query are case-SENSITIVE, so "/a/../b?x=1" stays
    // "/a/../b?x=1" — normalising those would change what the URL means).
    //   urlToString("HTTP://Example.COM/a/../b?x=1")
    //     == "http://example.com/a/../b?x=1"
    std::string urlToString(const std::string& url);

    // ── Argument counts 0–5 ──────────────────────────────────────────────────
    std::string noArgs();
    std::string oneArg(const std::string& a);
    std::string twoArgs(const std::string& a, int64_t b);
    std::string threeArgs(const std::string& a, int64_t b, bool c);
    std::string fourArgs(const std::string& a, int64_t b, bool c, const std::string& d);
    std::string fiveArgs(const std::string& a, int64_t b, bool c, const std::string& d, int64_t e);

    // ── Events ───────────────────────────────────────────────────────────────
    // Drivers kept as methods with their original names and arity: consumers
    // (test_ipc_module, test_ipc_new_api_module, the event-system group in
    // run_tests.sh) call them BY NAME to make this module emit.
    void emitTestEvent(const std::string& data);
    void emitMultiArgEvent(const std::string& name, int64_t count);

    // ── Async helpers ────────────────────────────────────────────────────────
    std::string echoWithDelay(const std::string& value, int64_t delayMs);

    // ── Crash testing ────────────────────────────────────────────────────────
    void crashOnDemand();

    // The two events this module emits. They were previously emitted
    // dynamically by name through the Qt `eventResponse` signal; declaring
    // them here keeps the SAME wire names ("testEvent" / "multiArgEvent")
    // while making them typed and discoverable by consumer-side codegen.
logos_events:
    void testEvent(const std::string& data);
    void multiArgEvent(const std::string& name, int64_t count);
};
