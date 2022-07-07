#pragma once

#include <Lib/Pool.h>
#include <Renderer/ResourceManager.h>

struct ID3D12Resource;

namespace nv::graphics
{
    class ShaderDX12;
    class GPUResourceDX12;
    class PipelineStateDX12;

    class ResourceManagerDX12 : public ResourceManager
    {
    public:
        // Inherited via ResourceManager
        virtual Handle<Shader> CreateShader(const ShaderDesc& desc) override;
        virtual Handle<GPUResource> CreateResource(const GPUResourceDesc& desc) override;
        virtual Handle<PipelineState> CreatePipelineState(const PipelineState& desc) override;

    public:
        // DX12 specific functions
        ID3D12Resource* GetResource(Handle<GPUResource> handle);

    private:
        Pool<GPUResource, GPUResourceDX12> mGpuResources;
        Pool<Shader, ShaderDX12> mShaders;
        Pool<PipelineState, PipelineStateDX12> mPipelineStates;
    };
}