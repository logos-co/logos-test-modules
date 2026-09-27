#include "test_probe_module_cpp_impl.h"

#include <logos_caller.h>

namespace {

const char* kindName(logos::CallerKind kind)
{
    switch (kind) {
    case logos::CallerKind::Host: return "host";
    case logos::CallerKind::Module: return "module";
    case logos::CallerKind::Derived: return "derived";
    case logos::CallerKind::Operator: return "operator";
    case logos::CallerKind::Unknown: break;
    }
    return "unknown";
}

} // namespace

void TestProbeModuleCppImpl::onContextReady()
{
    m_configuredBeforeContext = !configuration().empty();
}

std::string TestProbeModuleCppImpl::configurationText() const
{
    return configuration();
}

bool TestProbeModuleCppImpl::configuredBeforeContext() const
{
    return m_configuredBeforeContext;
}

LogosMap TestProbeModuleCppImpl::callerIdentity() const
{
    const logos::LogosCaller& caller = logos::currentCaller();
    return LogosMap{{"kind", kindName(caller.kind)}, {"name", caller.name}, {"scoped", caller.scoped}};
}

std::string TestProbeModuleCppImpl::ping() const
{
    return "pong";
}

std::string TestProbeModuleCppImpl::secret() const
{
    return "shh";
}
