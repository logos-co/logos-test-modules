#pragma once

// A DEPENDENCY INTERFACE — a method/event contract decoupled from any
// concrete module. It names NO module; any module whose own API is a
// superset of this can satisfy it. The consumer binds it to a concrete
// module name at runtime (modules().bind_basic_calc("some_module")).
//
// This file is the consuming module's own pure-C++ interface declaration
// (the same language the universal module is written in). The Logos
// generator parses it — public methods + the logos_events: block — and
// emits a BOUND wrapper class `BasicCalc` whose target module name is a
// runtime ctor argument rather than baked in.
//
// `test_basic_module_cpp` satisfies this interface: its API includes
// echo / addInts / returnTrue and a `testEvent(data)` event, plus much
// more (the superset rule).
//
// NOTE: types are std (std::string / int64_t) because the consuming
// module is `interface: "universal"`; the bound wrapper inherits that
// api-style.

#include <cstdint>
#include <string>

// Defines the `logos_events` token (it expands to `public`), so this header
// is valid C++ on its own — not just as generator input.
#include <logos_module_context.h>

class IBasicCalc {
public:
    // Echo a string back unchanged.
    std::string echo(const std::string& input);

    // Add two integers.
    int64_t addInts(int64_t a, int64_t b);

    // Always-true sentinel — proves a zero-arg bool method binds.
    bool returnTrue();

logos_events:
    // Emitted by the provider; the consumer subscribes via the bound
    // wrapper's generated onTestEvent(...) accessor.
    void testEvent(const std::string& data);
};
