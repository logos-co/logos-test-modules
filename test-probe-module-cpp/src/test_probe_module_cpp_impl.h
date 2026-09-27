#pragma once

// What the runtime gave this module, readable from outside: its configuration,
// whether that arrived before its context, and who is calling it.

#include <string>

#include <logos_json.h>            // LogosMap
#include <logos_module_context.h>  // configuration(), onContextReady()

class TestProbeModuleCppImpl : public LogosModuleContext {
public:
    TestProbeModuleCppImpl() = default;
    ~TestProbeModuleCppImpl() = default;

    void onContextReady() override;

    // The configuration document it was given, as JSON text; "" for none.
    std::string configurationText() const;
    // Whether that document was already there when onContextReady() fired.
    bool configuredBeforeContext() const;
    // This call's caller: {"kind", "name", "scoped"}, "scoped" being true only
    // when the runtime checked the call against a method-list grant.
    LogosMap callerIdentity() const;

    // Two methods a policy can grant or withhold.
    std::string ping() const;
    std::string secret() const;

private:
    bool m_configuredBeforeContext = false;
};
