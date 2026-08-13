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
    std::string reverseString(const std::string& input);
    std::string uppercaseString(const std::string& input);
    std::string lowercaseString(const std::string& input);

    // ── strutil counters ─────────────────────────────────────────────────────
    int64_t countChars(const std::string& input);
    int64_t countChar(const std::string& input, const std::string& ch);

    // ── strutil metadata ─────────────────────────────────────────────────────
    std::string libVersion();
};
