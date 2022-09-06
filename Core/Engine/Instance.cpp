#include "pch.h"
#include "Instance.h"
#include <Engine/Log.h>
#include <Math/Math.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/Window.h>
#include <Engine/JobSystem.h>
#include <Engine/Job.h>
#include <Engine/System.h>
#include <Engine/Timer.h>
#include <Debug/Profiler.h>

#include <thread>

namespace nv
{ 
    bool Instance::sError = false;
    const char* Instance::spErrorReason = nullptr;
    Timer gTimer;

    static std::atomic<InstanceState> gInstanceState = INSTANCE_STATE_STOPPED;

    bool Instance::Init()
    {
        NV_APP(mAppName);
        NV_EVENT("App/Init");
        log::Info("Init Nova App: {}", mAppName);
        nv::InitContext(this);
        graphics::InitGraphics();
        return true;
    }
    
    bool Instance::Run()
    {
        gInstanceState = INSTANCE_STATE_RUNNING;
        gTimer.Start();
        gSystemManager.InitSystems();

        while (UpdateSystemState())
        {
            NV_FRAME("MainThread");
            gTimer.Tick();
            {
                NV_EVENT("App/Update");
                //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                gSystemManager.UpdateSystems(gTimer.DeltaTime, gTimer.TotalTime);
            }
        }
        return true;
    }
    
    bool Instance::Destroy()
    {
        gSystemManager.DestroySystems();
        graphics::DestroyGraphics();
        nv::DestroyContext();
        NV_SHUTDOWN();
        return true;
    }

    void Instance::Reload()
    {
    }

    InstanceState Instance::GetInstanceState()
    {
        return gInstanceState.load();
    }

    void Instance::SetInstanceState(InstanceState state)
    {
        gInstanceState.store(state);
    }

    void Instance::SetError(bool isError, const char* pReason)
    {
        sError = isError;
        spErrorReason = pReason;
        gInstanceState = INSTANCE_STATE_ERROR;
    }

    bool Instance::UpdateSystemState() const
    {
        bool result = graphics::gWindow->ProcessMessages() != graphics::Window::kNvQuit;
        result = result || sError;
        if (!result) gInstanceState = INSTANCE_STATE_STOPPED;
        return result;
    }
}
