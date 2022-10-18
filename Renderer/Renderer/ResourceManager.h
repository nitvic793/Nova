#pragma once

#include <Lib/Handle.h>
#include <Lib/Pool.h>
#include <Renderer/CommonDefines.h>
#include <AssetBase.h>
#include <Renderer/ResourceTracker.h>

namespace nv::graphics
{
    struct ShaderDesc;
    struct GPUResourceDesc;
    struct PipelineStateDesc;
    struct TextureDesc;
    struct MeshDesc;
    struct ContextDesc;
    struct PBRMaterial;
    struct Material;

    class Shader;
    class GPUResource;
    class PipelineState;
    class Texture;
    class Mesh;
    class Context;

    class ResourceManager
    {
    public:
        ResourceManager();
        virtual Handle<Shader>          CreateShader(const ShaderDesc& desc) = 0;
        virtual Handle<GPUResource>     CreateResource(const GPUResourceDesc& desc) = 0;
        virtual Handle<GPUResource>     CreateEmptyResource() = 0; // Creates a resource pointer
        virtual Handle<PipelineState>   CreatePipelineState(const PipelineStateDesc& desc) = 0;
        virtual Handle<Texture>         CreateTexture(const TextureDesc& desc) = 0;
        virtual Handle<Texture>         CreateTexture(const TextureDesc& desc, ResID id);
        virtual Handle<Texture>         CreateTexture(asset::AssetID asset);
        virtual Handle<Mesh>            CreateMesh(const MeshDesc& desc) = 0;
        virtual Handle<Mesh>            CreateMesh(const MeshDesc& desc, ResID id);
        virtual Handle<Context>         CreateContext(const ContextDesc& desc) = 0;

        virtual GPUResource*            Emplace(Handle<GPUResource>& handle) = 0;

        Handle<GPUResource>             CreateResource(const GPUResourceDesc& desc, ResID id);
        Handle<Material>                CreateMaterial(const PBRMaterial& matDesc, ResID id);
        Handle<Material>                GetMaterialHandle(ResID id);
        Handle<Mesh>                    GetMeshHandle(ResID id);
        Handle<Texture>                 GetTextureHandle(ResID id);
        Handle<GPUResource>             GetGPUResourceHandle(ResID id);
        Material*                       GetMaterial(Handle<Material> handle);
        Material*                       GetMaterial(ResID id);


        virtual Texture*                GetTexture(Handle<Texture>) = 0;
        virtual Texture*                GetTexture(ResID id);
        virtual GPUResource*            GetGPUResource(Handle<GPUResource>) = 0;
        virtual PipelineState*          GetPipelineState(Handle<PipelineState>) = 0;
        virtual Shader*                 GetShader(Handle<Shader>) = 0;
        virtual Mesh*                   GetMesh(Handle<Mesh>) = 0;
        virtual Context*                GetContext(Handle<Context>) = 0;

        virtual Handle<PipelineState>   RecreatePipelineState(Handle<PipelineState> handle) = 0;

        virtual void                    DestroyResource(Handle<GPUResource> resource) = 0;

        virtual ~ResourceManager();

    private:
        Pool<Material> mMaterialPool;
    };

    extern ResourceManager* gResourceManager;
    extern ResourceTracker gResourceTracker;;
}