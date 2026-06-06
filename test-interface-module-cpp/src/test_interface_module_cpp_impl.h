#pragma once

// Integration smoke test for DEPENDENCY INTERFACES.
//
// Unlike test_context_module_cpp (which calls a concrete dependency via
// the name-baked modules().test_basic_module_cpp wrapper), this module
// declares an INTERFACE (interfaces/basic_calc.h) and binds it to a
// module name chosen at RUNTIME. The codegen turns that interface into a
// bound wrapper class `BasicCalc`, and the umbrella `LogosModules` gains
// a `bind_basic_calc(moduleName)` factory.
//
// The methods below take the target module name as a parameter so the
// integration runner can bind to a real provider ("test_basic_module_cpp")
// and to a bogus name (to prove the no-validation contract: a non-superset
// / missing module surfaces an ordinary remote-call error, not a crash).
//
// The impl header stays free of the generated logos_sdk.h (the codegen
// parses this file and expects only plain C++); the bind/call sites live
// in the .cpp, which includes logos_sdk.h.

#include <cstdint>
#include <string>

#include <logos_module_context.h>  // LogosModuleContext base (gives modules())

class TestInterfaceModuleCppImpl : public LogosModuleContext {
public:
    TestInterfaceModuleCppImpl() = default;
    ~TestInterfaceModuleCppImpl() = default;

    // Bind `basic_calc` to `moduleName` and call echo(input) through the
    // bound wrapper. Returns the echoed string.
    std::string echoVia(const std::string& moduleName, const std::string& input);

    // Bind `basic_calc` to `moduleName` and call addInts(a, b) through it.
    int64_t addVia(const std::string& moduleName, int64_t a, int64_t b);

    // Bind `basic_calc` to `moduleName` and call returnTrue(). Used as a
    // liveness probe: against a real provider returns true; against a
    // missing module the remote call yields the default (false) — no crash.
    bool probe(const std::string& moduleName);

    // Bind `basic_calc` to `moduleName` and subscribe to its `testEvent`
    // via the generated onTestEvent(...) accessor. Returns "ok" on
    // successful registration, "failed" otherwise.
    std::string subscribeTestEvent(const std::string& moduleName);

    // Last `testEvent(data)` payload captured by subscribeTestEvent()'s
    // callback. Empty until the event fires.
    std::string lastTestEvent() const;

private:
    std::string m_lastTestEvent;
};
