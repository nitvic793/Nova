#include "pch.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/RenderSystem.h>
#include <Engine/System.h>

#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
#include <DX12/WindowDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#endif

namespace nv::graphics
{
    IRenderer* gRenderer = nullptr;
    Window* gWindow = nullptr;

    void InitGraphics()
    {
#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
        gWindow = Alloc<WindowDX12>();
        gWindow->Init(1920, 1080);

        gRenderer = Alloc<RendererDX12>();
        gRenderer->Init(*gWindow);

        gSystemManager.CreateSystem<RenderSystem>();
#endif
    }

    void DestroyGraphics()
    {
        gRenderer->Destroy();
        Free(gRenderer);
        Free(gWindow);
    }
}
