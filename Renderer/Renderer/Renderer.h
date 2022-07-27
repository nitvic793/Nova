#pragma once

#include <Lib/Handle.h>
#include <Lib/ScopedPtr.h>
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

        virtual void Draw() {}
        virtual void Present() {}
        virtual void Destroy() = 0;
        virtual void Submit(Context* pContext) = 0;
        virtual ~IRenderer() {}

        Device* GetDevice() const;

    protected:
        ScopedPtr<Device, true> mDevice;
        Handle<GPUResource>     mpBackBuffers[FRAMEBUFFER_COUNT];
        Handle<Texture>         mRenderTargets[FRAMEBUFFER_COUNT];
        Handle<Texture>         mDepthStencil;
    };

    extern IRenderer* gRenderer;

    void InitGraphics(void* context = nullptr);
    void DestroyGraphics();
}