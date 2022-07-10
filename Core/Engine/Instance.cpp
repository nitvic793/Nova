#include "pch.h"
#include "Instance.h"
#include <Engine/Log.h>
#include <Math/Math.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/Window.h>

namespace nv
{ 
    bool Instance::Init()
    {
        log::Info("Init Nova App: {}", mAppName);
        log::Warn("Warn Log Test");
        log::Error("Error Log Test");
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
}
