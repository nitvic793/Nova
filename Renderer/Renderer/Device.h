#pragma once

#include <Lib/ScopedPtr.h>
#include <Renderer/Format.h>

namespace nv::graphics
{
    class Window;
    class SwapChain;
    class DescriptorHeap;

    struct RenderTargetDesc
    {

    };

    struct DepthStencilDesc
    {

    };

    class Device
    {
    public:
        virtual bool Init(Window& window) = 0;
        virtual void Present() = 0;
        virtual bool InitSwapChain(const Window& window, const format::SurfaceFormat format) = 0;
        virtual void InitRaytracingContext() = 0;
        virtual ~Device() {}

    public:


    protected:
    };
}