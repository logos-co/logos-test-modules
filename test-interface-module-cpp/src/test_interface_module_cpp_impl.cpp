#include "test_interface_module_cpp_impl.h"

// Generated at build time by logos-cpp-generator. Defines `LogosModules`
// with the name-baked dependency wrappers AND, because metadata.json lists
// `interface_dependencies`, a `bind_basic_calc(moduleName)` factory that
// returns the bound `BasicCalc` wrapper. Included only in the .cpp so the
// impl header the generator parses stays free of Qt/codegen types.
#include "logos_sdk.h"

std::string TestInterfaceModuleCppImpl::echoVia(const std::string& moduleName,
                                                const std::string& input) {
    // Bind the interface to a runtime-chosen module, then call it with the
    // usual typed accessor — no module name on the call itself.
    auto calc = modules().bind_basic_calc(moduleName);
    return calc.echo(input);
}

int64_t TestInterfaceModuleCppImpl::addVia(const std::string& moduleName,
                                           int64_t a, int64_t b) {
    auto calc = modules().bind_basic_calc(moduleName);
    return calc.addInts(a, b);
}

bool TestInterfaceModuleCppImpl::probe(const std::string& moduleName) {
    auto calc = modules().bind_basic_calc(moduleName);
    return calc.returnTrue();
}

std::string TestInterfaceModuleCppImpl::subscribeTestEvent(const std::string& moduleName) {
    // Subscribe through the bound wrapper's generated typed accessor. The
    // bound handle is a temporary, but the subscription is registered on
    // the underlying (LogosAPI-owned) client/replica and the callback
    // captures `this`, so it outlives the handle.
    bool ok = modules().bind_basic_calc(moduleName).onTestEvent(
        [this](const std::string& data) {
            m_lastTestEvent = data;
        });
    return ok ? "ok" : "failed";
}

std::string TestInterfaceModuleCppImpl::lastTestEvent() const {
    return m_lastTestEvent;
}
