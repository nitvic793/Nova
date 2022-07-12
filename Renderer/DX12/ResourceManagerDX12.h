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

    class ResourceManagerDX12 : public ResourceManager
    {
    public:
        ResourceManagerDX12();

        // Inherited via ResourceManager
        virtual Handle<Shader>          CreateShader(const ShaderDesc& desc) override;
        virtual Handle<GPUResource>     CreateResource(const GPUResourceDesc& desc) override;
        virtual Handle<PipelineState>   CreatePipelineState(const PipelineState& desc) override;
        virtual Handle<Texture>         CreateTexture(const TextureDesc& desc) override;
        virtual Handle<Mesh>            CreateMesh(const TextureDesc& desc) override;

        virtual Texture*                GetTexture(Handle<Texture>) override;
        virtual GPUResource*            GetGPUResource(Handle<GPUResource>) override;
        virtual PipelineState*          GetPipelineState(Handle<PipelineState>) override;
        virtual Shader*                 GetShader(Handle<Shader>) override;
        virtual Mesh*                   GetMesh(Handle<Mesh>) override;

    private:
        Pool<GPUResource, GPUResourceDX12>      mGpuResources;
        Pool<Shader, ShaderDX12>                mShaders;
        Pool<PipelineState, PipelineStateDX12>  mPipelineStates;
        Pool<Texture, TextureDX12>              mTextures;
        Pool<Mesh, MeshDX12>                    mMeshes;
    };
}