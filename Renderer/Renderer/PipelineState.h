#pragma once

#include <Lib/Handle.h>

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
    };

    class PipelineState
    {

    };
}