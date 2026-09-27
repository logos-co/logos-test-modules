#pragma once

// A provider declared `concurrency: multi`. sleepMs holds a call for a while;
// peakInFlight says how many were held at once since the last reset.

#include <atomic>
#include <cstdint>

#include <logos_module_context.h>

class TestConcurrencyCppImpl : public LogosModuleContext {
public:
    TestConcurrencyCppImpl() = default;
    ~TestConcurrencyCppImpl() = default;

    // Returns how many calls were in sleepMs when this one began, itself included.
    int64_t sleepMs(int64_t ms);
    int64_t peakInFlight();
    void reset();

private:
    std::atomic<int64_t> inFlight_{0};
    std::atomic<int64_t> peak_{0};
};
