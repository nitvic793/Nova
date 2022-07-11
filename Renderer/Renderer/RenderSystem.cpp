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
        gRenderer->Present();
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnReload()
    {
    }
}