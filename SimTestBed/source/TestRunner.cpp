#include "TestRunner.h"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <Engine/Log.h>
#include <Engine/Timer.h>
#include <Simulation.h>

namespace nv::sim
{
    class SimulationAPI
    {
        static constexpr const wchar_t* SIM_DLL = L"Simulation.dll";
        using FInit = decltype(NVSimInit)*;
        using FTick = decltype(NVSimTick)*;
        using FTickState = decltype(NVSimTickState)*;


    public:
        void Init()
        {
            LoadDLL();
        }

        void LoadDLL()
        {
            mInstance = LoadLibrary(SIM_DLL);

            if (!mInstance) {
                nv::log::Error("Unable to load Simulation DLL");
                return;
            }

            SimInit = (FInit)GetProcAddress(mInstance, "NVSimInit");
            Tick = (FTick)GetProcAddress(mInstance, "NVSimTick");
            TickState = (FTickState)GetProcAddress(mInstance, "NVSimTickState");
        }

        FInit SimInit;
        FTick Tick;
        FTickState TickState;

    private:
        HINSTANCE mInstance;
    };
}

namespace nv
{
    void TestRunner::Run()
    {
        nv::Timer timer;
        sim::SimulationAPI simApi;
        simApi.Init();

        simApi.SimInit();
        
        timer.Start();

        while (true)
        {
            simApi.Tick(timer.DeltaTime, timer.TotalTime);
        }
        
    }
}