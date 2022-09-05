#pragma once

#include <wrl/client.h>
#include <Renderer/Context.h>

struct ID3D12GraphicsCommandList4;
struct ID3D12CommandAllocator;
struct ID3D12Device;

namespace nv::graphics
{
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
        virtual void Begin() override;
        virtual void End() override;
        virtual void SetMesh(Handle<Mesh> mesh) override;
        virtual void SetPipeline(Handle<PipelineState> pipeline) override;
        virtual void SetRenderTarget(Span<Handle<Texture>> renderTargets, Handle<Texture> dsvHandle, bool singleRTV) override;
        virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) override;
        virtual void ResourceBarrier(Span<TransitionBarrier> barriers) override;
        virtual void UpdateSubresources(Handle<GPUResource> dest, Handle<GPUResource> staging, const SubResourceDesc& desc) override;
        virtual void ClearRenderTarget(Handle<Texture> renderTarget, float color[4], uint32_t numRects, Rect* pRects) override;
        virtual void ClearDepthStencil(Handle<Texture> depthStencil, float depth, uint8_t stencil, uint32_t numRects, Rect* pRects) override;
        virtual void SetViewports(uint32_t numViewports, Viewport* pViewports) override;
        virtual void SetScissorRect(uint32_t numRect, Rect* pRects) override;

    public:
        ID3D12GraphicsCommandList4* GetCommandList() const { return mCommandList.Get(); }

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        ComPtr<ID3D12GraphicsCommandList4>  mCommandList;
    };
}