#pragma once

#include <Lib/Handle.h>
#include <Lib/Pool.h>
#include <Renderer/CommonDefines.h>
#include <AssetBase.h>

#include <Renderer/ResourceTracker.h>
#include <mutex>
#include <Components/Material.h>
#include <Asset.h>

namespace nv::graphics
{
    struct ShaderDesc;
    struct GPUResourceDesc;
    struct PipelineStateDesc;
    struct TextureDesc;
    struct MeshDesc;
    struct ContextDesc;
    struct PBRMaterial;
    struct MaterialInstance;

    class Shader;
    class GPUResource;
    class PipelineState;
    class Texture;
    class Mesh;
    class Context;

    template<typename THandle>
    struct ResourceAsyncLoadRequest
    {
        Handle<THandle> mHandle;
        ResID           mResID;
    };

    template<typename THandle>
    using HandleQueue = std::vector<ResourceAsyncLoadRequest<THandle>>;

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

        virtual void                    CreateMesh(Handle<Mesh> handle, const MeshDesc& desc) = 0;
        virtual void                    CreateTexture(Handle<Texture> handle, const TextureDesc& desc) = 0;

        Handle<Texture>                 CreateTexture(asset::Asset& data);

        // Async Functions
        Handle<Mesh>                    CreateMeshAsync(ResID id);
        Handle<Texture>                 CreateTextureAsync(ResID id);

        virtual void                    CreatePipelineState(const PipelineStateDesc& desc, PipelineState* pPSO) = 0;

        virtual GPUResource*            Emplace(Handle<GPUResource>& handle) = 0;

        Handle<GPUResource>             CreateResource(const GPUResourceDesc& desc, ResID id);
        Handle<MaterialInstance>        CreateMaterial(const PBRMaterial& matDesc, ResID id);
        Handle<MaterialInstance>        CreateMaterial(const Material& matDesc, ResID id);

        Handle<MaterialInstance>        GetMaterialHandle(ResID id);
        Handle<Mesh>                    GetMeshHandle(ResID id);
        Handle<Texture>                 GetTextureHandle(ResID id);
        Handle<GPUResource>             GetGPUResourceHandle(ResID id);
        MaterialInstance*               GetMaterial(Handle<MaterialInstance> handle);
        MaterialInstance*               GetMaterial(ResID id);
        void                            DestroyTexture(ResID id);
        void                            QueueDestroy(Handle<GPUResource> handle, uint32_t frameDelay = 0);

        virtual Texture*                GetTexture(Handle<Texture>) = 0;
        virtual Texture*                GetTexture(ResID id);
        virtual GPUResource*            GetGPUResource(Handle<GPUResource>) = 0;
        virtual PipelineState*          GetPipelineState(Handle<PipelineState>) = 0;
        virtual Shader*                 GetShader(Handle<Shader>) = 0;
        virtual Mesh*                   GetMesh(Handle<Mesh>) = 0;
        virtual Context*                GetContext(Handle<Context>) = 0;

        virtual Handle<PipelineState>   RecreatePipelineState(Handle<PipelineState> handle) = 0;

        virtual void                    DestroyResource(Handle<GPUResource> resource) = 0;
        virtual void                    DestroyTexture(Handle<Texture> texture) = 0;

        virtual ~ResourceManager();

    public:
        virtual Handle<Mesh>            EmplaceMesh() = 0;
        virtual Handle<Texture>         EmplaceTexture() = 0;

        void                            ProcessAsyncLoadQueue();
        uint32_t                        GetAsyncLoadQueueSize() const;

    private:
        HandleQueue<Mesh>               mMeshQueue;
        HandleQueue<Texture>            mTextureQueue;
        std::mutex                      mMutex;

    private:
        Pool<MaterialInstance> mMaterialPool;
    };

    extern ResourceManager* gResourceManager;
    extern ResourceTracker gResourceTracker;;
}