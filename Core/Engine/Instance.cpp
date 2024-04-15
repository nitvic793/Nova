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
#include <Input/InputSystem.h>
#include <Input/Input.h>
#include <Debug/Profiler.h>

#include <thread>
#include <mutex>
#include <condition_variable>

namespace nv
{ 
    bool Instance::sError = false;
    const char* Instance::spErrorReason = nullptr;
    Timer gTimer;

    static std::atomic<InstanceState> gInstanceState = INSTANCE_STATE_STOPPED;
    static std::mutex gInstanceMutex;
    static std::condition_variable gInstanceCondVar;

    bool Instance::Init()
    {
        //NV_APP(mAppName);
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

        while (UpdateSystemState() && gInstanceState.load() == INSTANCE_STATE_RUNNING)
        {
            NV_FRAME_MARK();
            NV_FRAME("MainThread");
            gTimer.Tick();
            {
                NV_EVENT("App/Update");
                //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                gSystemManager.UpdateSystems(gTimer.DeltaTime, gTimer.TotalTime);
            }

            Notify(); // Notify end of frame to other threads
        }
        return true;
    }
    
    bool Instance::Destroy()
    {
        gSystemManager.DestroySystems();
        NV_SHUTDOWN();
        graphics::DestroyGraphics();
        nv::DestroyContext();
        return true;
    }

    void Instance::Reload()
    {
    }

    void Instance::Notify()
    {
        gInstanceCondVar.notify_all();
    }

    void Instance::Wait()
    {
        std::unique_lock<std::mutex> lock(gInstanceMutex);
        gInstanceCondVar.wait(lock);
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

        if (!result) 
            gInstanceState = INSTANCE_STATE_STOPPED;

        if (gInstanceState == INSTANCE_STATE_STOPPED)
            result = false;

        return result;
    }
}
