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

    class Context
    {
    public:
        Context(const ContextDesc& desc) :
            mDesc(desc) {}
        virtual ~Context() {}

        virtual void Begin() {}
        virtual void End() {}
        virtual void SetMesh(Handle<Mesh> mesh) = 0;
        virtual void SetPipeline(Handle<PipelineState> pipeline) = 0;
        virtual void SetRenderTarget(Span<Handle<Texture>> renderTargets, Handle<Texture> dsvHandle, bool singleRTV = false) = 0;
        virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;
        virtual void ResourceBarrier(Span<TransitionBarrier> barriers) = 0;
        virtual void UpdateSubresources(Handle<GPUResource> dest, Handle<GPUResource> staging, const SubResourceDesc& desc) = 0;

    protected:
        ContextDesc mDesc;
    };
}