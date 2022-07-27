#pragma once

#include <Lib/Pool.h>
#include <Renderer/Renderer.h>
#include <wrl/client.h>

struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;

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
        virtual void Submit(Context* pContext) override;
        ~RendererDX12();

    public:
        ID3D12CommandAllocator* GetAllocator() const;

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        Pool<DescriptorHeap, DescriptorHeapDX12, 8> mDescriptorHeapPool;
        Handle<DescriptorHeap>                      mRtvHeap;
        Handle<DescriptorHeap>                      mDsvHeap;
        Handle<DescriptorHeap>                      mGpuHeap;
        Handle<DescriptorHeap>                      mTextureHeap;
        Handle<DescriptorHeap>                      mConstantBufferHeap;
        ComPtr<ID3D12CommandAllocator>              mCommandAllocators[FRAMEBUFFER_COUNT];
        ComPtr<ID3D12CommandQueue>                  mCommandQueue;

        friend class ResourceManagerDX12;
    };

    void ReportLeaksDX12();
}
