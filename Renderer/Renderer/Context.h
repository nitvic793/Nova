#pragma once

#include <Lib/Handle.h>
#include <Lib/Vector.h>
#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    class Mesh;
    class PipelineState;
    class Texture;
    struct TransitionBarrier;

    struct ContextDesc
    {
        ContextType mType;
    };

    class CommandBuffer
    {

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

    protected:
        ContextDesc mDesc;
    };
}