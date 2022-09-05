#pragma once

#include <Lib/Handle.h>

namespace nv::graphics
{
    struct ShaderDesc;
    struct GPUResourceDesc;
    struct PipelineStateDesc;
    struct TextureDesc;
    struct MeshDesc;
    struct ContextDesc;

    class Shader;
    class GPUResource;
    class PipelineState;
    class Texture;
    class Mesh;
    class Context;

    class ResourceManager
    {
    public:
        virtual Handle<Shader>          CreateShader(const ShaderDesc& desc) = 0;
        virtual Handle<GPUResource>     CreateResource(const GPUResourceDesc& desc) = 0;
        virtual Handle<PipelineState>   CreatePipelineState(const PipelineStateDesc& desc) = 0;
        virtual Handle<Texture>         CreateTexture(const TextureDesc& desc) = 0;
        virtual Handle<Mesh>            CreateMesh(const MeshDesc& desc) = 0;
        virtual Handle<Context>         CreateContext(const ContextDesc& desc) = 0;

        virtual GPUResource*            Emplace(Handle<GPUResource>& handle) = 0;

        virtual Texture*                GetTexture(Handle<Texture>) = 0;
        virtual GPUResource*            GetGPUResource(Handle<GPUResource>) = 0;
        virtual PipelineState*          GetPipelineState(Handle<PipelineState>) = 0;
        virtual Shader*                 GetShader(Handle<Shader>) = 0;
        virtual Mesh*                   GetMesh(Handle<Mesh>) = 0;
        virtual Context*                GetContext(Handle<Context>) = 0;

        virtual ~ResourceManager() {}
    };

    extern ResourceManager* gResourceManager;
}