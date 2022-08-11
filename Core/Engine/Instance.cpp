#include "pch.h"
#include "Instance.h"
#include <Engine/Log.h>
#include <Math/Math.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/Window.h>
#include <Engine/JobSystem.h>
#include <Engine/Job.h>
#include <thread>

namespace nv
{ 
    bool Instance::sError = false;
    const char* Instance::spErrorReason = nullptr;

    bool Instance::Init()
    {
        log::Info("Init Nova App: {}", mAppName);
        nv::InitContext(this);
        graphics::InitGraphics();

        return true;
    }
    
    bool Instance::Run()
    {
        while (graphics::gWindow->ProcessMessages() != graphics::Window::kNvQuit)
        {

        }
        return true;
    }
    
    bool Instance::Destroy()
    {
        graphics::DestroyGraphics();
        nv::DestroyContext();
        return true;
    }

    void Instance::Reload()
    {
    }

    void Instance::SetError(bool isError, const char* pReason)
    {
        sError = isError;
        spErrorReason = pReason;
    }
}
