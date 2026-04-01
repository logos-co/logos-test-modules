#ifndef DUMMY_MODULE_000000_IMPL_H
#define DUMMY_MODULE_000000_IMPL_H

#include "logos_provider_object.h"

class DummyModule000000Impl : public LogosProviderBase
{
    LOGOS_PROVIDER(DummyModule000000Impl, "dummy_module_000000", "1.0.0")

public:
    LOGOS_METHOD void noop();
};

#endif // DUMMY_MODULE_000000_IMPL_H
