#pragma once

#include <Lib/Handle.h>
#include <Renderer/Format.h>
#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    class Shader;

    enum PipelineType
    {
        PIPELINE_RASTER,
        PIPELINE_COMPUTE,
        PIPELINE_RAYTRACING
    };

    struct PipelineStateDesc
    {
        PipelineType    mPipelineType;
        Handle<Shader>  mVS; // Vertex Shader
        Handle<Shader>  mPS; // Pixel Shader
        Handle<Shader>  mCS; // Compute Shader

        DepthStencilState mDepthStencilState;
        RasterizerState   mRasterizerState;
        BlendState        mBlendState;

        uint32_t                mNumRenderTargets   = 1;
        format::SurfaceFormat   mRenderTargetFormats[MAX_RENDER_TARGET_COUNT];
        format::SurfaceFormat   mDepthFormat        = format::D32_FLOAT;
        PrimitiveTopologyType   mPrimitiveTopology  = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        bool                    mbUseAnimLayout     = false;
    };

    class PipelineState
    {
    public:
        PipelineState(const PipelineStateDesc& desc) :
            mDesc(desc) {}
        virtual ~PipelineState() {}
        constexpr const PipelineStateDesc& GetDesc() const { return mDesc; }

    protected:
        PipelineStateDesc mDesc;
    };
}