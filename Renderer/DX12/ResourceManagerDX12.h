#pragma once

#include <Lib/Pool.h>
#include <Renderer/ResourceManager.h>

struct ID3D12Resource;

namespace nv::graphics
{
    class ShaderDX12;
    class GPUResourceDX12;
    class PipelineStateDX12;
    class TextureDX12;
    class MeshDX12;
    class DeviceDX12;
    class ContextDX12;

    class ResourceManagerDX12 : public ResourceManager
    {
    public:
        ResourceManagerDX12();

        // Inherited via ResourceManager
        virtual Handle<Shader>          CreateShader(const ShaderDesc& desc) override;
        virtual Handle<GPUResource>     CreateResource(const GPUResourceDesc& desc) override;
        virtual Handle<GPUResource>     CreateEmptyResource() override;
        virtual Handle<PipelineState>   CreatePipelineState(const PipelineStateDesc& desc) override;
        virtual Handle<Texture>         CreateTexture(const TextureDesc& desc) override;
        virtual Handle<Mesh>            CreateMesh(const MeshDesc& desc) override;
        virtual Handle<Context>         CreateContext(const ContextDesc& desc) override;
        virtual void                    CreateMesh(Handle<Mesh> handle, const MeshDesc& desc) override;
        virtual void                    CreateTexture(Handle<Texture> handle, const TextureDesc& desc) override;

        virtual void                    CreatePipelineState(const PipelineStateDesc& desc, PipelineState* pPSO) override;
        virtual GPUResource*            Emplace(Handle<GPUResource>& handle) override;

        virtual Texture*                GetTexture(Handle<Texture>) override;
        virtual GPUResource*            GetGPUResource(Handle<GPUResource>) override;
        virtual PipelineState*          GetPipelineState(Handle<PipelineState>) override;
        virtual Shader*                 GetShader(Handle<Shader>) override;
        virtual Mesh*                   GetMesh(Handle<Mesh>) override;
        virtual Context*                GetContext(Handle<Context>) override;

        virtual Handle<PipelineState>   RecreatePipelineState(Handle<PipelineState> handle) override;
        virtual void                    DestroyResource(Handle<GPUResource> resource) override;
        virtual void                    DestroyTexture(Handle<Texture> resource) override;
        ~ResourceManagerDX12();

    public:
        virtual Handle<Mesh>            EmplaceMesh() override;
        virtual Handle<Texture>         EmplaceTexture() override;

    private:
        Pool<GPUResource, GPUResourceDX12>      mGpuResources;
        Pool<Shader, ShaderDX12>                mShaders;
        Pool<PipelineState, PipelineStateDX12>  mPipelineStates;
        Pool<Texture, TextureDX12>              mTextures;
        Pool<Mesh, MeshDX12>                    mMeshes;
        Pool<Context, ContextDX12>              mContexts;
        DeviceDX12*                             mDevice;
        friend class RendererDX12;
    };
}