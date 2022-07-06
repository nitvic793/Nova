#include "pch.h"
#include "Instance.h"
#include <Engine/Log.h>
#include <Math/Math.h>

namespace nv
{ 
    bool Instance::Init()
    {
        log::Info("Init Nova App: {}", mAppName);
        log::Warn("Warn Log Test");
        log::Error("Error Log Test");
        nv::InitContext(this);
        return true;
    }
    
    bool Instance::Run()
    {
        return true;
    }
    
    bool Instance::Destroy()
    {
        nv::DestroyContext();
        return true;
    }

    void Instance::Reload()
    {
    }
}
