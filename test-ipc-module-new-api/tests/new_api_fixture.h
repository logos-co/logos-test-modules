#pragma once

// Test wiring for a universal (`interface: "universal"`) module impl.
//
// In production the generated Qt plugin glue calls the framework-only
// `_logosCoreSet*_` seams on the impl before dispatching anything. A unit test
// has no host and no plugin, so it must do the same by hand. This fixture is
// that wiring, in one place.
//
// It replaces the old `LogosAPI api(...); impl.init(&api);` two-liner, which
// belonged to the LogosProviderBase authoring path that no longer exists.
//
// NOTE `LogosModuleContext::modules()` blindly dereferences the injected
// pointer — calling a dep-forwarding method without this fixture is undefined
// behaviour, not a graceful failure. Every test constructs one.

#include "logos_sdk.h"  // generated: struct LogosModules (lp-typed)
#include "test_ipc_new_api_impl.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

struct ImplFixture {
    // Declaration order is load-bearing: members destruct in reverse, so the
    // umbrella must be declared BEFORE the impl that points at it.
    LogosModules modules;
    TestIpcNewApiImpl impl;

    struct EmittedEvent {
        std::string name;
        nlohmann::json args;
    };
    std::vector<EmittedEvent> events;

    ImplFixture()
    {
        impl._logosCoreSetLogosModulesPtr_(&modules);

        // Typed `logos_events:` bodies route through emitEventImpl_, which is a
        // silent no-op when no callback is installed. Installing one here is
        // what lets a test assert an event was actually emitted rather than
        // merely that the method returned.
        impl._logosCoreSetEmitEvent_([this](const std::string& name, void* args) {
            events.push_back({
                name,
                args ? *static_cast<nlohmann::json*>(args) : nlohmann::json(),
            });
        });
    }

    TestIpcNewApiImpl* operator->() { return &impl; }
};
