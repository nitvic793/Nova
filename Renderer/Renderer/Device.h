#pragma once

#include <Lib/ScopedPtr.h>

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
        virtual ~Device() {}

    public:


    protected:
    };
}