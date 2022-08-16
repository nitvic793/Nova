#include "pch.h"
#include "Timer.h"

#define NOMINMAX
#include <Windows.h>

namespace nv
{
	Timer* Timer::Instance = nullptr;

    void Timer::Start()
    {
		__int64 perfFreq;
		QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
		perfCounterSeconds = 1.0 / (double)perfFreq;

		__int64 now;
		QueryPerformanceCounter((LARGE_INTEGER*)&now);
		startTime = now;
		currentTime = now;
		previousTime = now;
		frameCounter = 0;

		DeltaTime = 0.f;
		TotalTime = 0.f;
    }

    void Timer::Tick()
    {
		frameCounter++;
		__int64 now;
		QueryPerformanceCounter((LARGE_INTEGER*)&now);
		currentTime = now;

		DeltaTime = std::max((float)((currentTime - previousTime) * perfCounterSeconds), 0.f);
		TotalTime = (float)((currentTime - startTime) * perfCounterSeconds);
		previousTime = currentTime;
    }
}
