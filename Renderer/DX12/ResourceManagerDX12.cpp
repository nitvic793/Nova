#include "pch.h"

#include "ResourceManagerDX12.h"

#include <DX12/ShaderDX12.h>
#include <DX12/GPUResourceDX12.h>
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

    ID3D12Resource* ResourceManagerDX12::GetResource(Handle<GPUResource> handle)
    {
        return nullptr;
    }
}
