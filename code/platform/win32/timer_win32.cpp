#include "common_win32.h"

platform_timer::platform_timer()
{
    LARGE_INTEGER FrequencyResult;
    QueryPerformanceFrequency(&FrequencyResult);

    mFrequency   = 1.0 / (f64)FrequencyResult.QuadPart;
    mElapsedTime = 0.0;
    mStartTime   = 0.0;
}

void
platform_timer::Start()
{
    LARGE_INTEGER QueryResult{};
    QueryPerformanceCounter(&QueryResult);
    mStartTime = QueryResult.QuadPart * mFrequency;
}

void 
platform_timer::Update()
{
    LARGE_INTEGER QueryResult{};
    QueryPerformanceCounter(&QueryResult);
    mElapsedTime = QueryResult.QuadPart * mFrequency;
}

f64 
platform_timer::GetSecondsElapsed() const
{
    return mElapsedTime - mStartTime;
}
