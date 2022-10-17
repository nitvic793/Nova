#pragma once

#include <Lib/Handle.h>
#include <Lib/Vector.h>
#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    class Mesh;
    class PipelineState;
    class Texture;
    class GPUResource;
    class DescriptorHeap;

    struct TransitionBarrier;

    struct ContextDesc
    {
        ContextType mType;
    };

    struct SubResourceDesc
    {
        void*       mpData              = nullptr;
        int64_t     mRowPitch           = 0;
        int64_t     mSlicePitch         = 0;
        uint32_t    mNumSubresources    = 1;
        uint32_t    mFirstSubresource   = 0;
        uint64_t    mIntermediateOffset = 0;
    };


    enum BindResourceType
    {
        BIND_BUFFER,
        BIND_TEXTURE
    };

    class Context
    {
    public:
        Context(const ContextDesc& desc) :
            mDesc(desc) {}
        virtual ~Context() {}

        virtual void InitRaytracingContext() = 0;
        virtual void Begin() {}
        virtual void End() {}
        virtual void SetMesh(Handle<Mesh> mesh) = 0;
        virtual void SetMesh(Mesh* mesh) = 0;
        virtual void SetPipeline(Handle<PipelineState> pipeline) = 0;
        virtual void SetRenderTarget(Span<Handle<Texture>> renderTargets, Handle<Texture> dsvHandle, bool singleRTV = false) = 0;
        virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;
        virtual void ResourceBarrier(Span<TransitionBarrier> barriers) = 0;
        virtual void UpdateSubresources(Handle<GPUResource> dest, Handle<GPUResource> staging, const SubResourceDesc& desc) = 0;
        virtual void ClearRenderTarget(Handle<Texture> renderTarget, float color[4], uint32_t numRects, Rect* pRects) = 0;
        virtual void ClearDepthStencil(Handle<Texture> depthStencil, float depth, uint8_t stencil, uint32_t numRects, Rect* pRects) = 0;
        virtual void SetViewports(uint32_t numViewports, Viewport* pViewports) = 0;
        virtual void SetScissorRect(uint32_t numRect, Rect* pRects) = 0;
        virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;
        virtual void SetDescriptorHeap(Span<Handle<DescriptorHeap>> heaps) = 0;
        virtual void Bind(uint32_t slot, BindResourceType type, uint32_t offset) = 0;
        virtual void BindConstantBuffer(uint32_t slot, uint32_t offset) = 0;
        virtual void BindTexture(uint32_t slot, Handle<Texture> texture) = 0;

    protected:
        ContextDesc mDesc;
    };
}