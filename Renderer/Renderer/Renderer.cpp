#include "pch.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/RenderSystem.h>
#include <Engine/System.h>
#include <Renderer/ResourceManager.h>

#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
#include <DX12/WindowDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/ResourceManagerDX12.h>
#endif

namespace nv::graphics
{
    IRenderer* gRenderer = nullptr;
    Window* gWindow = nullptr;
    ResourceManager* gResourceManager = nullptr;
    ResourceManager* ResourceManager::gPtr = nullptr;

    void InitGraphics(void* context)
    {
#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
        gWindow = Alloc<WindowDX12>(SystemAllocator::gPtr, (HWND)context);
        gWindow->Init(1280, 720);

        gRenderer = Alloc<RendererDX12>();
        gRenderer->Init(*gWindow);

        gResourceManager = Alloc<ResourceManagerDX12>();
        ResourceManager::gPtr = gResourceManager;

        gSystemManager.CreateSystem<RenderSystem>();
#endif
    }

    void DestroyGraphics()
    {
        gRenderer->Destroy();
        Free(gResourceManager);
        Free(gRenderer);
        Free(gWindow);

#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12 && _DEBUG
        ReportLeaksDX12();
#endif
    }
}
