#pragma once

#include <Lib/Handle.h>
#include <Lib/ScopedPtr.h>
#include <Lib/Vector.h>
#include <Renderer/CommonDefines.h>
#include <Renderer/Format.h>

namespace nv::graphics
{
    class Shader;
    class Texture;
    class GPUResource;
    class Device;
    class Window;
    class DescriptorHeap;
    class Context;

    class IRenderer
    {
    public:
        virtual void Init(Window& window) = 0;
        virtual void InitFrameBuffers(const Window& window, const format::SurfaceFormat format) = 0;

        virtual void Draw() = 0;
        virtual void Present() = 0;
        virtual void Destroy() = 0;
        virtual void Submit(Context* pContext) = 0;
        virtual void Wait() = 0;
        virtual void WaitForAllFrames() = 0;
        virtual void ClearBackBuffers() = 0;

        virtual void TransitionToRenderTarget() = 0;
        virtual void TransitionToPresent() = 0;
        virtual void StartFrame() = 0;
        virtual void EndFrame() = 0;
        virtual ~IRenderer() {}

        virtual Handle<Texture>         GetDefaultRenderTarget() const = 0; // Return current final default render target
        virtual Handle<Texture>         GetDefaultDepthTarget() const = 0; // Return current final default depth target
        virtual format::SurfaceFormat   GetDepthSurfaceFormat() const = 0;
        virtual format::SurfaceFormat   GetDefaultRenderTargetFormat() const = 0;
        virtual Handle<DescriptorHeap>  GetGPUDescriptorHeap() const = 0;
        virtual Context*                GetContext() const = 0;
        virtual ConstantBufferView      CreateConstantBuffer(uint32_t size) = 0;
        virtual void                    UploadToConstantBuffer(ConstantBufferView view, uint8_t* data, uint32_t size) = 0;
        virtual uint32_t                GetHeapIndex(const ConstantBufferView& cbv) = 0;

        Device*     GetDevice() const;
        void        QueueDestroy(Handle<GPUResource> resource);
        void        ExecuteQueuedDestroy();
        Viewport    GetDefaultViewport() const;
        Rect        GetDefaultScissorRect() const;

    protected:
        ScopedPtr<Device, true>         mDevice;
        Handle<GPUResource>             mpBackBuffers[FRAMEBUFFER_COUNT];
        Handle<Texture>                 mRenderTargets[FRAMEBUFFER_COUNT];
        Handle<Texture>                 mDepthStencil;
        nv::Vector<Handle<GPUResource>> mDeleteQueue;
    };

    extern IRenderer* gRenderer;

    void InitGraphics(void* context = nullptr);
    void DestroyGraphics();
}