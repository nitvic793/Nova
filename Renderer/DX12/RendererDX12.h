#pragma once

#include <Lib/Pool.h>
#include <Renderer/Renderer.h>

namespace nv::graphics
{
    class DescriptorHeapDX12;

    class RendererDX12 : public IRenderer
    {
    public:
        // Inherited via IRenderer
        virtual void Init(Window& window) override;
        virtual void Destroy() override;
        virtual void Present() override;

    private:
        Pool<DescriptorHeap, DescriptorHeapDX12> mDescriptorHeapPool;
    };

    void ReportLeaksDX12();
}
