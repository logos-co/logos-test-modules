#include "test_concurrency_cpp_impl.h"

#include <algorithm>
#include <chrono>
#include <thread>

int64_t TestConcurrencyCppImpl::sleepMs(int64_t ms)
{
    const int64_t now = ++inFlight_;
    int64_t seen = peak_.load();
    while (now > seen && !peak_.compare_exchange_weak(seen, now)) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(std::clamp<int64_t>(ms, 0, 10000)));
    --inFlight_;
    return now;
}

int64_t TestConcurrencyCppImpl::peakInFlight()
{
    return peak_.load();
}

void TestConcurrencyCppImpl::reset()
{
    peak_ = 0;
}
