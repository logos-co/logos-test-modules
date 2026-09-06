#include "test_optional_module_cpp_impl.h"

// Generated at build time. `LogosModules` carries a name-baked wrapper for
// every CONCRETE dependency — `dependencies` and `optional_dependencies`
// alike, since the two differ in lifetime and not in call shape.
#include "logos_sdk.h"

std::string TestOptionalModuleCppImpl::callEcho(const std::string& input,
                                                int64_t timeoutMs) {
    logos::CallError err;
    std::string out = modules().test_basic_module_cpp.echo(
        input, &err, static_cast<int>(timeoutMs));
    // Report the failure CLASS rather than a bare empty string. "not running"
    // and "ran and returned nothing" are different answers, and a test that
    // could not tell them apart would pass against a module that had silently
    // stopped answering.
    if (!err.ok()) return "ABSENT:" + err.code;
    return out;
}

std::string TestOptionalModuleCppImpl::selfCheck() {
    return "alive";
}
