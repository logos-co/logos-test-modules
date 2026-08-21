#include "test_unload_module_cpp_impl.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace {

// How long the Asynchronous modes spend "working". Comfortably longer than an
// unwaiting host would give us (teardown without the hook settles in well under
// a second), and comfortably shorter than the host's grace period -- so
// `async` finishing is unambiguous evidence the host waited, rather than a
// race that happens to land the right way.
constexpr int kAsyncWorkMs = 1500;

// Longer than any grace period the host might reasonably use. The point is not
// the number: it is that this mode never finishes, so the daemon exiting at all
// proves the deadline is enforced rather than merely configured.
constexpr int kHangMs = 60000;

std::string envOr(const char* name, const char* fallback)
{
    const char* v = std::getenv(name);
    return (v && *v) ? std::string(v) : std::string(fallback);
}

// Appends one line. Opened and closed per write so a hard kill mid-teardown
// still leaves everything written up to that instant on disk -- which is
// exactly the evidence the `hang` case depends on.
void journal(const char* what)
{
    const char* path = std::getenv("LOGOS_UNLOAD_JOURNAL");
    if (!path || !*path) return;
    if (FILE* f = std::fopen(path, "a")) {
        std::fprintf(f, "%s\n", what);
        std::fclose(f);
    }
}

} // namespace

std::string TestUnloadModuleCppImpl::unloadMode()
{
    return envOr("LOGOS_UNLOAD_MODE", "sync");
}

std::string TestUnloadModuleCppImpl::unloadJournalPath()
{
    return envOr("LOGOS_UNLOAD_JOURNAL", "");
}

LogosShutdown TestUnloadModuleCppImpl::aboutToUnload()
{
    journal("ENTERED");
    const std::string mode = unloadMode();

    if (mode == "sync") {
        // Nothing outstanding. The host proceeds immediately.
        return LogosShutdown::Synchronous;
    }

    const int workMs = (mode == "hang") ? kHangMs : kAsyncWorkMs;

    // Detached on purpose: aboutToUnload() must RETURN so the host can start
    // waiting. Doing the work inline here would block the very event loop that
    // has to deliver the completion, and would make Asynchronous a slower
    // spelling of Synchronous.
    std::thread([this, workMs] {
        std::this_thread::sleep_for(std::chrono::milliseconds(workMs));
        journal("FINISHED");
        unloadFinished();
    }).detach();

    return LogosShutdown::Asynchronous;
}
