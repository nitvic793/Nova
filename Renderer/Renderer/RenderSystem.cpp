#include "pch.h"
#include "RenderSystem.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>

namespace nv::graphics
{
    void RenderSystem::Init()
    {
    }

    void RenderSystem::Update(float deltaTime, float totalTime)
    {
        gRenderer->Wait();
        gRenderer->StartFrame();

        gRenderer->EndFrame();
        gRenderer->Present();
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnReload()
    {
    }
}