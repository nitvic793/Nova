#include "pch.h"
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/RenderSystem.h>
#include <Renderer/ResourceSystem.h>
#include <Engine/System.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Format.h>
#include <Renderer/GlobalRenderSettings.h>
#include <Types/MeshAsset.h>
#include <AssetManager.h>
#include <Animation/AnimationSystem.h>

#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
#include <DX12/WindowDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/ContextDX12.h>
#include <d3d12.h>
#endif

#if NV_ENABLE_DEBUG_UI
#include <DebugUI/Imgui/imgui.h>
#endif

namespace nv::graphics
{
    IRenderer*          gRenderer           = nullptr;
    Window*             gWindow             = nullptr;
    ResourceManager*    gResourceManager    = nullptr;
    Handle<ecs::Entity> gActiveCamHandle    = Null<ecs::Entity>();
    Settings            gRenderSettings     = {};

    IRenderReloadManager* gRenderReloadManager = nullptr;

    void InitRenderSettings()
    {
        gRenderSettings.mbEnableVSync = true;
    }

    void InitGraphics(void* context)
    {
        uint32_t width = 1920;
        uint32_t height = 1080;
        const bool isFullscreen = false;

#if NV_PLATFORM_WINDOWS && NV_RENDERER_DX12
        gWindow = Alloc<WindowDX12>(SystemAllocator::gPtr, (HWND)context);
        gWindow->Init(width, height, isFullscreen);

        gRenderer = Alloc<RendererDX12>();
        gRenderer->Init(*gWindow);

        gResourceManager = Alloc<ResourceManagerDX12>();
        gRenderer->InitFrameBuffers(*gWindow, format::R8G8B8A8_UNORM); // Dependent on resource manager. 

        gSystemManager.CreateSystem<RenderSystem>(width, height);
        gSystemManager.CreateSystem<ResourceSystem>();
#endif
        gSystemManager.CreateSystem<animation::AnimationSystem>();
        InitRenderSettings();
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

#if NV_ENABLE_DEBUG_UI
    bool IsDebugUIInputActive()
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool debugNav = (io.NavActive || ImGui::IsAnyItemFocused());
        return debugNav;
    }
#endif

    void SetActiveCamera(Handle<ecs::Entity> camHandle)
    {
        gActiveCamHandle = camHandle;
    }

    Handle<ecs::Entity> GetActiveCamera()
    {
        return gActiveCamHandle;
    }

    IRenderReloadManager* GetRenderReloadManager()
    {
        return gRenderReloadManager;
    }

    void SetRenderReloadManager(IRenderReloadManager* pManager)
    {
        gRenderReloadManager = pManager;
    }

    Device* IRenderer::GetDevice() const
    {
        return mDevice.Get();
    }

    void IRenderer::QueueDestroy(Handle<GPUResource> resource, uint32_t frameDelay)
    {
        gResourceTracker.Remove(resource);
        mDeleteQueue.push_back({ resource, frameDelay });
    }

    void IRenderer::ExecuteQueuedDestroy()
    {
        const auto size = mDeleteQueue.size();
        uint32_t idxCounter = 0;
        while (!mDeleteQueue.empty())
        {
            if (idxCounter >= size)
                break;

            auto entry = *mDeleteQueue.begin();
            if (entry.mFrameDelay > 0)
            {
                entry.mFrameDelay--;
                mDeleteQueue.erase(mDeleteQueue.begin());
                mDeleteQueue.push_back(entry);
            }
            else
            {
                gResourceManager->DestroyResource(entry.mResource);
                mDeleteQueue.erase(mDeleteQueue.begin());
            }

            idxCounter++;
        }
    }

    Viewport IRenderer::GetDefaultViewport() const
    {
        return
        {
            .mTopLeftX = 0,
            .mTopLeftY = 0,
            .mWidth = (float)gWindow->GetWidth(),
            .mHeight = (float)gWindow->GetHeight(),
            .mMinDepth = 0.0f,
            .mMaxDepth = 1.0f
        };
    }

    Rect IRenderer::GetDefaultScissorRect() const
    {
        return
        {
            .mLeft = 0,
            .mTop = 0,
            .mRight = (int32_t)gWindow->GetWidth(),
            .mBottom = (int32_t)gWindow->GetHeight()
        };
    }
}
