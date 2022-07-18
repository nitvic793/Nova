#include "pch.h"
#include "TextureDX12.h"
#include <d3d12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/ResourceManagerDX12.h>

namespace nv::graphics
{ 
    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::GetCPUHandle() const
    {
        return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mCpuHandle);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::GetGPUHandle() const
    {
        return static_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(mGpuHandle);
    }

    DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) :
        mType(CPU)
    {
        mCpuHandle = static_cast<DescriptorHandle::CPUHandle>(handle.ptr);
    }

    DescriptorHandle::DescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) :
        mType(GPU)
    {
        mGpuHandle = static_cast<DescriptorHandle::GPUHandle>(handle.ptr);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE TextureDX12::GetCPUHandle() const
    {
        return mDescriptorHandle.GetCPUHandle();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE TextureDX12::GetGPUHandle() const
    {
        return mDescriptorHandle.GetGPUHandle();
    }

    ID3D12Resource* TextureDX12::GetResource() const
    {
        return ((GPUResourceDX12*)gResourceManager->GetGPUResource(mDesc.mBuffer))->GetResource().Get();
    }
}
