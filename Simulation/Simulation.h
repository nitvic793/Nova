#pragma once

#ifdef SIMULATION_EXPORTS
#define DLL_EXPORT __declspec(dllexport) 
#else 
#define DLL_EXPORT
#endif

extern "C"
{
	DLL_EXPORT void NVSimInit();
	DLL_EXPORT void NVSimTick(float deltaTime, float totalTime);
	DLL_EXPORT int  NVSimTickState();
}