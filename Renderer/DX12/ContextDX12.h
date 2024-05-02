#pragma once

#include <wrl/client.h>
#include <Renderer/Context.h>

struct ID3D12GraphicsCommandList4;
struct ID3D12GraphicsCommandList5;
struct ID3D12CommandAllocator;
struct ID3D12RootSignature;
struct ID3D12Device;

namespace nv::graphics
{
    class RendererDX12;

    class ContextDX12 : public Context
    {
    public:
        ContextDX12() :
            Context({}) {}
        ContextDX12(const ContextDesc& desc) :
            Context(desc) {}

        bool Init(ID3D12Device* pDevice, ID3D12CommandAllocator* pCommandAllocator);
        void Begin(ID3D12CommandAllocator* pCommandAllocator);

        // Inherited via Context
        virtual void InitRaytracingContext() override;
        virtual void Begin() override;
        virtual void End() override;
        virtual void SetMesh(Handle<Mesh> mesh) override;
        virtual void SetMesh(Mesh* mesh) override;
        virtual void SetPipeline(Handle<PipelineState> pipeline) override;
        virtual void SetRenderTarget(Span<Handle<Texture>> renderTargets, Handle<Texture> dsvHandle, bool singleRTV) override;
        virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) override;
        virtual void ResourceBarrier(Span<TransitionBarrier> barriers) override;
        virtual void ResourceBarrier(Span<UAVBarrier> barriers) override;
        virtual void UpdateSubresources(Handle<GPUResource> dest, Handle<GPUResource> staging, const SubResourceDesc& desc) override;
        virtual void ClearRenderTarget(Handle<Texture> renderTarget, float color[4], uint32_t numRects, Rect* pRects) override;
        virtual void ClearDepthStencil(Handle<Texture> depthStencil, float depth, uint8_t stencil, uint32_t numRects, Rect* pRects) override;
        virtual void SetViewports(uint32_t numViewports, Viewport* pViewports) override;
        virtual void SetScissorRect(uint32_t numRect, Rect* pRects) override;
        virtual void SetPrimitiveTopology(PrimitiveTopology topology) override;
        virtual void SetDescriptorHeap(Span<Handle<DescriptorHeap>> heaps) override;
        virtual void Bind(uint32_t slot, BindResourceType type, uint32_t offset) override;
        virtual void BindConstantBuffer(uint32_t slot, uint32_t offset) override;
        virtual void BindTexture(uint32_t slot, Handle<Texture> texture) override;
        virtual void CopyResource(Handle<GPUResource> dest, Handle<GPUResource> src) override;

        // Compute
        virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;
        virtual void ComputeBind(uint32_t slot, BindResourceType type, uint32_t offset) override;
        virtual void ComputeBindConstantBuffer(uint32_t slot, uint32_t offset) override;
        virtual void ComputeBindTexture(uint32_t slot, Handle<Texture> texture) override;
        virtual void ComputeBindTextures(uint32_t slot, Span<Handle<Texture>> textures) override;

    public:
        ID3D12GraphicsCommandList4* GetCommandList() const { return mCommandList.Get(); }
        ID3D12GraphicsCommandList5* GetDXRCommandList() const { return mDXRCommandList.Get(); }
        void SetRootSignature(ID3D12RootSignature* pRootSig);
        void SetComputeRootSignature(ID3D12RootSignature* pRootSig);

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        ComPtr<ID3D12GraphicsCommandList4>  mCommandList;
        ComPtr<ID3D12GraphicsCommandList5>  mDXRCommandList;

        friend class RendererDX12;
    };
}