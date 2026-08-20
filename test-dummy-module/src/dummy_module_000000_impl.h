#pragma once

// Dummy test module — the binary template the module-generation tests stamp
// out. Deliberately the smallest possible module: one no-op method, no
// dependencies, no events.
//
// `interface: "universal"`: this plain C++ class IS the module's API. The
// generator parses this header to derive the LIDL contract, then emits the
// Qt plugin glue and the C-ABI exports. There is no hand-written plugin
// loader and no Qt in this file.

#include <logos_module_context.h>  // LogosModuleContext base

class DummyModule000000Impl : public LogosModuleContext {
public:
    DummyModule000000Impl() = default;
    ~DummyModule000000Impl() = default;

    void noop();
};
