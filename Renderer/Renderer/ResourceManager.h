#pragma once

#include <Lib/Handle.h>

namespace nv::graphics
{
    struct ShaderDesc;
    struct GPUResourceDesc;
    struct PipelineStateDesc;

    class Shader;
    class GPUResource;
    class PipelineState;

    class ResourceManager
    {
    public:
        virtual Handle<Shader>          CreateShader(const ShaderDesc& desc) = 0;
        virtual Handle<GPUResource>     CreateResource(const GPUResourceDesc& desc) = 0;
        virtual Handle<PipelineState>   CreatePipelineState(const PipelineState& desc) = 0;
    };
}