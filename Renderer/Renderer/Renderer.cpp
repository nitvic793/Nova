#include "pch.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/RenderSystem.h>
#include <Engine/System.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Format.h>
#include <Types/MeshAsset.h>
#include <AssetManager.h>

#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
#include <DX12/WindowDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/ContextDX12.h>
#include <d3d12.h>
#endif

namespace nv::graphics
{
    IRenderer*          gRenderer           = nullptr;
    Window*             gWindow             = nullptr;
    ResourceManager*    gResourceManager    = nullptr;

    void InitGraphics(void* context)
    {
        uint32_t width = 1280;
        uint32_t height = 720;

#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
        gWindow = Alloc<WindowDX12>(SystemAllocator::gPtr, (HWND)context);
        gWindow->Init(width, height);

        gRenderer = Alloc<RendererDX12>();
        gRenderer->Init(*gWindow);

        gResourceManager = Alloc<ResourceManagerDX12>();
        gRenderer->InitFrameBuffers(*gWindow, format::R8G8B8A8_UNORM); // Dependent on resource manager. 

        gSystemManager.CreateSystem<RenderSystem>(width, height);
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

    Device* IRenderer::GetDevice() const
    {
        return mDevice.Get();
    }

    void IRenderer::QueueDestroy(Handle<GPUResource> resource)
    {
        mDeleteQueue.Push(resource);
    }

    void IRenderer::ExecuteQueuedDestroy()
    {
        for (auto res : mDeleteQueue)
        {
            gResourceManager->DestroyResource(res);
        }

        mDeleteQueue.Clear();
    }
}
