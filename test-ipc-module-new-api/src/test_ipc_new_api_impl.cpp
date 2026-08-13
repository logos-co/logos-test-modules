#include "test_ipc_new_api_impl.h"

#include <logos_sdk.h>  // generated: modules().<dep> typed wrappers

// Every method here forwards through the generated type-safe wrappers. The
// previous implementation used LogosAPIClient::invokeRemoteMethod with a
// module name and method name as strings; a typo in either was a runtime
// failure returning a null QVariant. These are compile-time checked.

// ── Calls to test_basic_module ───────────────────────────────────────────────

std::string TestIpcNewApiImpl::callBasicEcho(const std::string& input)
{
    return modules().test_basic_module.echo(input);
}

int64_t TestIpcNewApiImpl::callBasicAddInts(int64_t a, int64_t b)
{
    return modules().test_basic_module.addInts(a, b);
}

bool TestIpcNewApiImpl::callBasicReturnTrue()
{
    return modules().test_basic_module.returnTrue();
}

std::string TestIpcNewApiImpl::callBasicNoArgs()
{
    return modules().test_basic_module.noArgs();
}

std::string TestIpcNewApiImpl::callBasicFiveArgs(const std::string& a, int64_t b, bool c,
                                                 const std::string& d, int64_t e)
{
    return modules().test_basic_module.fiveArgs(a, b, c, d, e);
}

StdLogosResult TestIpcNewApiImpl::callBasicSuccessResult()
{
    return modules().test_basic_module.successResult();
}

StdLogosResult TestIpcNewApiImpl::callBasicErrorResult()
{
    return modules().test_basic_module.errorResult();
}

std::string TestIpcNewApiImpl::callBasicResultMapField(const std::string& key)
{
    // The value is a JSON object; return the named field as a string, or an
    // empty string when the call failed or the key is absent. Mirrors the old
    // QVariantMap lookup, which also yielded an empty QString in both cases.
    StdLogosResult r = modules().test_basic_module.resultWithMap();
    if (!r.success || !r.value.is_object()) return {};
    auto it = r.value.find(key);
    if (it == r.value.end()) return {};
    return it->is_string() ? it->get<std::string>() : it->dump();
}

// ── Calls to test_extlib_module ──────────────────────────────────────────────

std::string TestIpcNewApiImpl::callExtlibReverse(const std::string& input)
{
    return modules().test_extlib_module.reverseString(input);
}

std::string TestIpcNewApiImpl::callExtlibUppercase(const std::string& input)
{
    return modules().test_extlib_module.uppercaseString(input);
}

int64_t TestIpcNewApiImpl::callExtlibCountChars(const std::string& input)
{
    return modules().test_extlib_module.countChars(input);
}

// ── Cross-module chaining ────────────────────────────────────────────────────

std::string TestIpcNewApiImpl::chainEchoThenReverse(const std::string& input)
{
    return modules().test_extlib_module.reverseString(
        modules().test_basic_module.echo(input));
}

std::string TestIpcNewApiImpl::chainUppercaseThenConcat(const std::string& a,
                                                        const std::string& b)
{
    const std::string upperA = modules().test_extlib_module.uppercaseString(a);
    const std::string upperB = modules().test_extlib_module.uppercaseString(b);
    return modules().test_basic_module.concat(upperA, upperB);
}

// ── Typed wrappers ───────────────────────────────────────────────────────────

std::string TestIpcNewApiImpl::wrapperBasicEcho(const std::string& input)
{
    return modules().test_basic_module.echo(input);
}

std::string TestIpcNewApiImpl::wrapperExtlibReverse(const std::string& input)
{
    return modules().test_extlib_module.reverseString(input);
}

// ── Events ───────────────────────────────────────────────────────────────────

void TestIpcNewApiImpl::triggerBasicEvent(const std::string& data)
{
    modules().test_basic_module.emitTestEvent(data);
    triggeredBasicEvent(data);
}
