#pragma once

#include <Lib/Pool.h>
#include <Renderer/Renderer.h>
#include <Renderer/Format.h>
#include <wrl/client.h>

struct ID3D12RootSignature;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Fence;
using HANDLE = void*;

namespace nv::graphics
{
    class DescriptorHeapDX12;
    class Context;

    class RendererDX12 : public IRenderer
    {
    public:
        // Inherited via IRenderer
        virtual void Init(Window& window) override;
        virtual void Destroy() override;
        virtual void Present() override;
        virtual void InitFrameBuffers(const Window& window, const format::SurfaceFormat format) override;
        virtual void Submit(Context* pContext) override;
        virtual void Wait() override;
        virtual void ClearBackBuffers() override;
        virtual void Draw() override;

        virtual void TransitionToRenderTarget() override;
        virtual void TransitionToPresent() override;
        virtual void StartFrame() override;
        virtual void EndFrame() override;

        virtual Context* GetContext() const override;

        ~RendererDX12();

    public:

        uint32_t                GetBackBufferIndex() const;
        ID3D12CommandAllocator* GetAllocator() const;
        ID3D12CommandQueue*     GetCommandQueue() const;

    private:
        void                    CreateRootSignature();

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        Pool<DescriptorHeap, DescriptorHeapDX12, 8> mDescriptorHeapPool;
        Handle<DescriptorHeap>                      mRtvHeap;
        Handle<DescriptorHeap>                      mDsvHeap;
        Handle<DescriptorHeap>                      mGpuHeap;
        Handle<DescriptorHeap>                      mTextureHeap;
        Handle<DescriptorHeap>                      mConstantBufferHeap;
        Handle<Context>                             mContexts[FRAMEBUFFER_COUNT];
        ComPtr<ID3D12CommandAllocator>              mCommandAllocators[FRAMEBUFFER_COUNT];
        ComPtr<ID3D12CommandQueue>                  mCommandQueue;
        ComPtr<ID3D12RootSignature>                 mRootSignature;

        ComPtr<ID3D12Fence>                         mFences[FRAMEBUFFER_COUNT];
        uint64_t                                    mFenceValues[FRAMEBUFFER_COUNT];
        HANDLE                                      mFenceEvents[FRAMEBUFFER_COUNT];

        format::SurfaceFormat                       mDsvFormat;
        format::SurfaceFormat                       mBackbufferFormat;

        friend class ResourceManagerDX12;
    };

    void ReportLeaksDX12();
}
