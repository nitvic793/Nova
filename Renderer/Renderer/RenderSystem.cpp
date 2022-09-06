#include "pch.h"
#include "RenderSystem.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Engine/JobSystem.h>
#include <Engine/Instance.h>
#include <Engine/Log.h>
#include <Debug/Profiler.h>

#include <thread>
#include <functional>

namespace nv::graphics
{
    void RenderSystem::Init()
    {
        mRenderJobHandle = nv::jobs::Execute([this](void* ctx) 
        { 
            nv::log::Info("[Renderer] Start Render Job");
            RenderThreadJob(ctx); 
        });
    }

    void RenderSystem::Update(float deltaTime, float totalTime)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }

    void RenderSystem::Destroy()
    {
        nv::jobs::Wait(mRenderJobHandle);
        gRenderer->Wait();
    }

    void RenderSystem::OnReload()
    {
    }

    void RenderSystem::RenderThreadJob(void* ctx)
    {
        while (Instance::GetInstanceState() == INSTANCE_STATE_RUNNING)
        {
            NV_FRAME("RenderThread");
            gRenderer->Wait();
            gRenderer->StartFrame();

            gRenderer->EndFrame();
            gRenderer->Present();
        }

        
    }
}