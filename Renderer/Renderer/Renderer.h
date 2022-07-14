#pragma once

#include <Lib/Handle.h>
#include <Lib/ScopedPtr.h>
#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    class Shader;
    class GPUResource;
    class Device;
    class Window;
    class DescriptorHeap;

    class IRenderer
    {
    public:
        virtual void Init(Window& window) = 0;
        virtual void InitDependentResources() {};

        virtual void Draw() {}
        virtual void Present() {}
        virtual void Destroy() = 0;
        virtual ~IRenderer() {}

        Device* GetDevice() const;

    protected:
        ScopedPtr<Device, true> mDevice;
        Handle<GPUResource>     mpBackBuffers[FRAMEBUFFER_COUNT];
    };

    extern IRenderer* gRenderer;

    void InitGraphics(void* context = nullptr);
    void DestroyGraphics();
}