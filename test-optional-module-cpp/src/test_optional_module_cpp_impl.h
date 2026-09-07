#pragma once

// Integration smoke test for OPTIONAL DEPENDENCIES.
//
// metadata.json declares `test_basic_module_cpp` under
// `optional_dependencies` and declares NOTHING as required. That is the whole
// setup, and it is what makes the three properties observable from outside:
//
//   1. This module loads on its own. `logoscore -l test_optional_module_cpp`
//      must NOT pull test_basic_module_cpp in, and must not fail for its
//      absence — which is what separates an optional dependency from a
//      required one.
//   2. The typed wrapper exists anyway. `modules().test_basic_module_cpp` is
//      generated from the dependency's published LIDL exactly as a required
//      dependency's is; the name is concrete, so the contract is. If that were
//      not true this file would not compile.
//   3. Absence is REPORTED, not waited out. See callEcho's `timeoutMs` — the
//      caller decides what absence costs, which is the point: nothing local
//      can prove a process-isolated module is gone, so a bounded call IS the
//      check. (`modules_state.is_ready` can answer authoritatively, but that
//      is a dependency of its own and not what this module is testing.)
//
// The runner drives both halves: once with only this module loaded, and once
// with the dependency loaded alongside it.

#include <cstdint>
#include <string>

#include <logos_module_context.h>  // LogosModuleContext base (gives modules())

class TestOptionalModuleCppImpl : public LogosModuleContext {
public:
    TestOptionalModuleCppImpl() = default;
    ~TestOptionalModuleCppImpl() = default;

    // A typed call through the optional dependency, bounded by `timeoutMs`.
    //
    // Returns the echoed string on success, or "ABSENT:<code>" when the call
    // failed — `object_unavailable` for a module that is not running. The
    // deadline is a PARAMETER because it is the caller's decision: absence
    // costs whatever deadline you set, and the point of this method is that a
    // consumer can make that cost small.
    std::string callEcho(const std::string& input, int64_t timeoutMs);

    // Proof that the module itself is alive and answering, independent of the
    // dependency. The first assertion of the absent-dependency run.
    std::string selfCheck();
};
