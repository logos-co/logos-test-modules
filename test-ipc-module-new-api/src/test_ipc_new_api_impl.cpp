#include "test_ipc_new_api_impl.h"

#include <chrono>
#include <future>
#include <memory>
#include <utility>

#include <logos_sdk.h>  // generated: modules().<dep> typed wrappers

namespace {

// Drive one `<name>Async(args..., callback)` wrapper and block until it fires.
//
// The promise is held by SHARED pointer, not by reference into this frame. On
// the timeout path below the frame goes away while the call is still
// outstanding, and a late completion then writes through whatever it was given
// — a reference would make that a write to a dead promise, i.e. the timeout
// path would corrupt memory precisely when something is already wrong.
//
// Bounded rather than infinite. An async wrapper that never fires would
// otherwise surface as the harness's own 30s timeout with no indication of
// WHICH call stalled; returning the sentinel turns it into an ordinary failed
// assertion naming the method.
template <typename T, typename Start>
T awaitAsync(Start&& start)
{
    auto box = std::make_shared<std::promise<T>>();
    auto fut = box->get_future();
    start([box](T v) { box->set_value(std::move(v)); });
    if (fut.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
        return T{};
    return fut.get();
}

}  // namespace

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

// ── Async calls ──────────────────────────────────────────────────────────────
// The async half of the SAME generated wrappers the sync methods above use;
// on this (lp) surface `<name>Async` bottoms out in lp_invoke_async, so these
// exercise Qt-free async delivery end to end rather than only its signature.

std::string TestIpcNewApiImpl::asyncCallBasicEcho(const std::string& input)
{
    return awaitAsync<std::string>([&](auto cb) {
        modules().test_basic_module.echoAsync(input, std::move(cb));
    });
}

int64_t TestIpcNewApiImpl::asyncCallBasicAddInts(int64_t a, int64_t b)
{
    return awaitAsync<int64_t>([&](auto cb) {
        modules().test_basic_module.addIntsAsync(a, b, std::move(cb));
    });
}

std::string TestIpcNewApiImpl::asyncCallExtlibReverse(const std::string& input)
{
    return awaitAsync<std::string>([&](auto cb) {
        modules().test_extlib_module.reverseStringAsync(input, std::move(cb));
    });
}

std::string TestIpcNewApiImpl::asyncWrapperBasicEcho(const std::string& input)
{
    // Identical to asyncCallBasicEcho by construction — see the header. Kept as
    // its own method because the ipc test group calls it by name, and because
    // its Qt-consumer predecessor was the one async case that ALREADY went
    // through a generated wrapper; keeping the name keeps that lineage visible.
    return awaitAsync<std::string>([&](auto cb) {
        modules().test_basic_module.echoAsync(input, std::move(cb));
    });
}

// ── Events ───────────────────────────────────────────────────────────────────

void TestIpcNewApiImpl::triggerBasicEvent(const std::string& data)
{
    modules().test_basic_module.emitTestEvent(data);
    triggeredBasicEvent(data);
}
