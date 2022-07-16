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
        virtual void InitFrameBuffers(const Window& window, const format::SurfaceFormat format) override;
        ~RendererDX12();

    private:
        Pool<DescriptorHeap, DescriptorHeapDX12, 8> mDescriptorHeapPool;
        Handle<DescriptorHeap> mRtvHeap;
        Handle<DescriptorHeap> mDsvHeap;
        Handle<DescriptorHeap> mGpuHeap;
        Handle<DescriptorHeap> mTextureHeap;
        Handle<DescriptorHeap> mConstantBufferHeap;
    };

    void ReportLeaksDX12();
}
