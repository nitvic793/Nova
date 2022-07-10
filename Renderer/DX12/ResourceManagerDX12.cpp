#include "pch.h"

#include "ResourceManagerDX12.h"

#include <DX12/ShaderDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/PipelineStateDX12.h>
#include <DX12/TextureDX12.h>
#include <DX12/MeshDX12.h>
#include <d3d12.h>

namespace nv::graphics
{
    Handle<Shader> ResourceManagerDX12::CreateShader(const ShaderDesc& desc)
    {
        return Handle<Shader>();
    }

    Handle<GPUResource> ResourceManagerDX12::CreateResource(const GPUResourceDesc& desc)
    {
        return Handle<GPUResource>();
    }

    Handle<PipelineState> ResourceManagerDX12::CreatePipelineState(const PipelineState& desc)
    {
        return Handle<PipelineState>();
    }

    Handle<Texture> ResourceManagerDX12::CreateTexture(const TextureDesc& desc)
    {
        return Handle<Texture>();
    }

    Handle<Mesh> ResourceManagerDX12::CreateMesh(const TextureDesc& desc)
    {
        return Handle<Mesh>();
    }

    Texture* ResourceManagerDX12::GetTexture(Handle<Texture> handle)
    {
        return mTextures.Get(handle);
    }

    GPUResource* ResourceManagerDX12::GetGPUResource(Handle<GPUResource> handle)
    {
        return mGpuResources.Get(handle);
    }
    
    PipelineState* ResourceManagerDX12::GetPipelineState(Handle<PipelineState> handle)
    {
        return mPipelineStates.Get(handle);
    }

    Shader* ResourceManagerDX12::GetShader(Handle<Shader> handle)
    {
        return mShaders.Get(handle);
    }

    Mesh* ResourceManagerDX12::GetMesh(Handle<Mesh> handle)
    {
        return mMeshes.Get(handle);
    }
}
