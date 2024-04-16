#pragma once

#include <Lib/Pool.h>
#include <Renderer/Renderer.h>
#include <Renderer/Format.h>
#include <wrl/client.h>

struct ID3D12RootSignature;
struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12Resource;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
enum D3D12_DESCRIPTOR_HEAP_TYPE : int32_t;
using HANDLE = void*;
using D3D12_GPU_VIRTUAL_ADDRESS = uint64_t;

namespace nv::graphics
{
    class DescriptorHeapDX12;
    class Context;
    class GPUConstantBuffer;

    struct ConstantBufferState
    {
        uint64_t mCurrentMemoryOffset   = 0;
        uint32_t mCurrentCount          = 0;
    };



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
        virtual void WaitForAllFrames() override;
        virtual void ClearBackBuffers() override;
        virtual void Draw() override;

        virtual void TransitionToRenderTarget() override;
        virtual void TransitionToPresent() override;
        virtual void StartFrame() override;
        virtual void EndFrame() override;
        virtual void OnResize(const Window& window) override;

        virtual Handle<Texture>         GetDefaultDepthTarget() const override;
        virtual Handle<Texture>         GetDefaultRenderTarget() const override;
        virtual Handle<DescriptorHeap>  GetGPUDescriptorHeap() const override;
        virtual Context*                GetContext() const override;
        virtual ConstantBufferView      CreateConstantBuffer(ConstBufferDesc desc) override;
        virtual void                    UploadToConstantBuffer(ConstantBufferView view, uint8_t* data, uint32_t size) override;
        virtual format::SurfaceFormat   GetDepthSurfaceFormat() const override { return mDsvFormat; }
        virtual format::SurfaceFormat   GetDefaultRenderTargetFormat() const override { return mBackbufferFormat; }
        virtual uint32_t                GetHeapIndex(const ConstantBufferView& cbv) override;
        virtual void                    SetContextDefaults(Context* context) const override;
        virtual void                    SetComputeContextDefaults(Context* context) const override;
        ~RendererDX12();

    public:
        uint32_t                    GetBackBufferIndex() const;
        ID3D12CommandAllocator*     GetAllocator() const;
        ID3D12CommandQueue*         GetCommandQueue() const;
        ID3D12Resource*             GetConstBuffer() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetConstBufferHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetRTDescriptorHandle(uint32_t index) const;
        const GpuHeapState&         GetGPUHeapState() const override { return mGpuHeapState; }
        D3D12_GPU_VIRTUAL_ADDRESS   GetConstBufferAddress(const ConstantBufferView& view);
        DescriptorHeapDX12*         CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorCount, Handle<DescriptorHeap>& handle, bool shaderVisible = false);
        DescriptorHeapDX12*         GetDescriptorHeap(Handle<DescriptorHeap> handle) const;
        ID3D12RootSignature*        GetComputeRootSignature() const { return mComputeRootSignature.Get(); }
        D3D12_GPU_DESCRIPTOR_HANDLE AllocateDescriptors(nv::Span<Handle<Texture>> textures);

    private:
        void                    CreateRootSignature();
        void                    CopyDescriptorsToGPU();

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        Pool<DescriptorHeap, DescriptorHeapDX12>    mDescriptorHeapPool;
        Handle<DescriptorHeap>                      mRtvHeap;
        Handle<DescriptorHeap>                      mDsvHeap;
        Handle<DescriptorHeap>                      mGpuHeap;
        Handle<DescriptorHeap>                      mTextureHeap;
        Handle<DescriptorHeap>                      mConstantBufferHeap;
        Handle<DescriptorHeap>                      mRayTracingHeap;
        Handle<Context>                             mContexts[FRAMEBUFFER_COUNT];
        Handle<Context>                             mComputeContexts[FRAMEBUFFER_COUNT];
        ComPtr<ID3D12CommandAllocator>              mCommandAllocators[FRAMEBUFFER_COUNT];
        ComPtr<ID3D12CommandAllocator>              mComputeCommandAllocators[FRAMEBUFFER_COUNT];
        ComPtr<ID3D12CommandQueue>                  mCommandQueue;
        ComPtr<ID3D12RootSignature>                 mRootSignature;
        ComPtr<ID3D12RootSignature>                 mComputeRootSignature;

        ComPtr<ID3D12Fence>                         mFences[FRAMEBUFFER_COUNT];
        uint64_t                                    mFenceValues[FRAMEBUFFER_COUNT];
        HANDLE                                      mFenceEvents[FRAMEBUFFER_COUNT];

        format::SurfaceFormat                       mDsvFormat;
        format::SurfaceFormat                       mBackbufferFormat;

        GpuHeapState                                mGpuHeapState;
        ConstantBufferState                         mCbState;
        GPUConstantBuffer*                          mConstantBuffer;

        friend class ResourceManagerDX12;
        friend class ContextDX12;
    };

    void ReportLeaksDX12();
}
