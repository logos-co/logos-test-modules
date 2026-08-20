#pragma once

// The teardown contract, made observable.
//
// A module gets one chance to finish work before it is torn down:
// LogosModuleContext::aboutToUnload() (logos-cpp-sdk). Returning Synchronous
// means "already quiescent"; returning Asynchronous means "wait for me", and
// the module then calls unloadFinished() when it is done. The host waits, but
// only for a bounded grace period.
//
// WHY A JOURNAL FILE AND NOT STDERR. The subprocess container closes the
// child's stdout/stderr BEFORE it sends the stop signal, so anything a module
// prints during teardown is never relayed. A probe on stderr looks exactly
// like a hook that never fired -- which is the wrong lesson to teach a reader
// and the wrong signal to build a test on. This module appends to the file
// named by LOGOS_UNLOAD_JOURNAL instead, which survives the process.
//
// WHY THE MODE IS RUNTIME, NOT BUILD-TIME. All three behaviours ship in one
// module, chosen from LOGOS_UNLOAD_MODE at teardown. The runner flips an
// environment variable between daemon launches instead of building three
// modules, so the cases cannot drift apart in the ways three near-identical
// sources always do.
//
// WHAT THE JOURNAL PROVES, WITHOUT TIMING. The assertions are about which
// lines appear, not how long anything took:
//
//   sync   -> ENTERED           the hook is reached at all
//   async  -> ENTERED, FINISHED the host WAITED: work completes well after the
//                               point an unwaiting host would have killed us,
//                               so FINISHED cannot be written unless it waited
//   hang   -> ENTERED, no more  the deadline is enforced: the module asked to
//                               wait, never finished, and the daemon still exited
//
// That last pair is the whole contract -- a module that asks is waited for, a
// module that hangs costs a bounded delay and is torn down anyway -- and none
// of it depends on a wall-clock threshold that would flake in CI.

#include <string>

#include <logos_module_context.h>

class TestUnloadModuleCppImpl : public LogosModuleContext {
public:
    /// The teardown mode this module will use, as read from LOGOS_UNLOAD_MODE.
    /// Callable so a test can confirm the module agrees with the environment
    /// before the interesting part happens -- an empty journal is otherwise
    /// ambiguous between "the hook never fired" and "the mode was never set".
    std::string unloadMode();

    /// Absolute path of the journal, or "" when LOGOS_UNLOAD_JOURNAL is unset.
    /// Lets a runner assert it is looking at the file the module will write.
    std::string unloadJournalPath();

protected:
    LogosShutdown aboutToUnload() override;
};
