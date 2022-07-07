#pragma once

#include <Lib/Handle.h>
#include <Renderer/Shader.h>
#include <Renderer/GPUResource.h>

namespace nv::graphics
{
    class ResourceManager
    {
    public:
        virtual Handle<Shader> CreateShader(const ShaderDesc& desc) = 0;
        virtual Handle<GPUResource> CreateResource(const GPUResourceDesc& desc) = 0;
    };
}