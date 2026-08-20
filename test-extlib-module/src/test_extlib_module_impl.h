#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// test_extlib_module — universal C++ module wrapping an external C library.
//
// The point of this fixture is the *external library* wiring: CMakeLists
// builds `lib/libstrutil.c` into a static `strutil` target and links it in
// via `LINK_TARGETS`. That part is unchanged by the move to
// `interface: "universal"`; what changed is that the module no longer carries
// a hand-written Qt plugin. This plain C++ class IS the contract — the
// generator parses this header to derive the LIDL and emits the Qt glue,
// the dispatch table and the C-ABI exports into generated_code/.
//
// The method surface is deliberately UNCHANGED across the migration (same
// names, same arity, same wire types) except that `int` becomes 64-bit, which
// is what LIDL numbers have always been on the wire.
//
// No Qt anywhere in this module's own translation units. The strutil header
// is included by the .cpp only — it has no bearing on the contract, and
// keeping it out of here keeps the parsed surface to exactly the methods.
//
// ── The unit of a "character" ────────────────────────────────────────────────
// Every method below that talks about characters means ONE UNICODE CODE
// POINT, and a `std::string` here is always UTF-8. That is the whole
// definition; there is no second, byte-shaped one.
//
// strutil is a BYTE library: strutil_count_chars is strlen, strutil_reverse
// reverses bytes, strutil_count_char compares single bytes. Byte answers and
// character answers coincide for ASCII and diverge for everything else, and
// what this module used to do was hand strutil's byte answers straight back:
// reverseString("héllo") produced a byte sequence that is not even valid UTF-8
// (the Qt module ahead of it ran those bytes through QString::fromUtf8, which
// substitutes U+FFFD for them; the first universal port failed the call
// outright at serialisation — measured, not inferred).
//
// The methods below return the CHARACTER answers. Where a strutil call can
// still carry the work without producing a wrong or ill-formed result it is
// kept, because exercising that external C library is what this fixture is
// for; each method states which part still goes through it.
//
// NO trailing `// comments` on declaration lines (the parser needs a `;`).
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <string>

#include <logos_module_context.h>  // LogosModuleContext base

class TestExtlibModuleImpl : public LogosModuleContext {
public:
    TestExtlibModuleImpl() = default;
    ~TestExtlibModuleImpl() = default;

    // ── strutil string transforms ────────────────────────────────────────────

    // Reverse `input` by CHARACTER: reverseString("héllo") == "olléh", and the
    // result is always well-formed UTF-8 when the input is. strutil_reverse
    // still performs the reversal (it is the only thing that touches the byte
    // order); this module then puts each multi-byte sequence back in reading
    // order, which byte reversal necessarily left backwards.
    std::string reverseString(const std::string& input);

    // ASCII-only case mapping, performed entirely by strutil_uppercase /
    // strutil_lowercase. Bytes >= 0x80 are passed through untouched, so
    // multi-byte characters survive intact but are NOT case-mapped:
    // uppercaseString("héllo") == "HéLLO". Real Unicode case mapping needs a
    // case table this fixture has no reason to carry.
    //
    // That pass-through is only true because strutil now compares 'a'..'z'
    // explicitly. It used to call toupper()/tolower(), which are
    // LOCALE-DEPENDENT: in en_US.UTF-8 they map ~30 of the 128 high bytes,
    // including the UTF-8 lead bytes 0xC3/0xE4/0xF0 — so lowercaseString("HÉLLO")
    // returned invalid UTF-8 and the call failed outright. Do not reintroduce
    // <ctype.h> here; the asserted non-ASCII round-trips below are the guard.
    std::string uppercaseString(const std::string& input);
    std::string lowercaseString(const std::string& input);

    // ── strutil counters ─────────────────────────────────────────────────────

    // Number of CHARACTERS in `input`: countChars("héllo") == 5, and an
    // emoji outside the BMP counts 1. strutil_count_chars supplies the byte
    // length; this module discounts the UTF-8 continuation bytes.
    int64_t countChars(const std::string& input);

    // Occurrences of `ch` in `input`, where `ch` is a STRING and every
    // character of it must match: countChar("héllo", "é") == 1,
    // countChar("banana", "na") == 2. Matches are counted left to right and
    // do NOT overlap — countChar("aaa", "aa") == 1 — and only ever start on a
    // character boundary, so no match can ever be part of a character.
    // An empty `ch` matches nothing and the answer is 0.
    // A single ASCII `ch` is still counted by strutil_count_char: an ASCII
    // byte cannot occur inside a multi-byte UTF-8 sequence, so for that case
    // its byte comparison IS a character comparison.
    int64_t countChar(const std::string& input, const std::string& ch);

    // ── strutil metadata ─────────────────────────────────────────────────────
    std::string libVersion();
};
