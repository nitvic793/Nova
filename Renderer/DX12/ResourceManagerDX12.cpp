#include "pch.h"

#include "ResourceManagerDX12.h"
#include <DX12/RendererDX12.h>
#include <DX12/ShaderDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/PipelineStateDX12.h>
#include <DX12/TextureDX12.h>
#include <DX12/MeshDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/Interop.h>
#include <d3d12.h>
#include "d3dx12.h"

namespace nv::graphics
{
    ResourceManagerDX12::ResourceManagerDX12()
    {
        mDevice = (DeviceDX12*)gRenderer->GetDevice();
        mGpuResources.Init();
        mMeshes.Init();
        mPipelineStates.Init();
        mShaders.Init();
        mTextures.Init();
    }

    Handle<Shader> ResourceManagerDX12::CreateShader(const ShaderDesc& desc)
    {
        return Handle<Shader>();
    }

    Handle<GPUResource> ResourceManagerDX12::CreateResource(const GPUResourceDesc& desc)
    {
        ID3D12Device* device = mDevice->GetDevice();

        Handle<GPUResource> handle;
        auto resource = (GPUResourceDX12*)mGpuResources.CreateInstance(handle);
        if (!resource)
            return Null<GPUResource>();
        
        ID3D12Resource* d3dResource = nullptr;
        CD3DX12_RESOURCE_DESC bufferDesc = {};
        CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_HEAP_FLAGS heapFlag = D3D12_HEAP_FLAG_NONE;
        D3D12_CLEAR_VALUE clearValue = {};
        const D3D12_RESOURCE_STATES initResourceState = GetState(desc.mInitialState);
        const DXGI_FORMAT format = GetFormat(desc.mFormat);
        const D3D12_RESOURCE_FLAGS flags = GetFlags(desc.mFlags);

        switch (desc.mType)
        {
        case buffer::TYPE_BUFFER:
            bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.mWidth);
            heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            break;
        case buffer::TYPE_TEXTURE_2D: 
            bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, desc.mWidth, desc.mHeight, desc.mArraySize, desc.mMipLevels, desc.mSampleCount, desc.mSampleQuality, flags);
            break;
        }

        auto hr = device->CreateCommittedResource(&heapProps, heapFlag, &bufferDesc, initResourceState, &clearValue, IID_PPV_ARGS(resource->GetResource().ReleaseAndGetAddressOf()));
        if (!SUCCEEDED(hr)) return Null<GPUResource>();

        return handle;
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
