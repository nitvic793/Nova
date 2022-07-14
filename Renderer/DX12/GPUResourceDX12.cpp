#include "pch.h"
#include "GPUResourceDX12.h"
#include "DirectXIncludes.h"

using namespace Microsoft::WRL;

namespace nv::graphics
{
    void GPUResourceDX12::SetResource(ID3D12Resource* pResource) noexcept
    {
        mResource.Attach(pResource);
    }

    ComPtr<ID3D12Resource>& GPUResourceDX12::GetResource() noexcept
    {
        return mResource;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::GetCPUHandle() const
    {
        return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mCpuHandle);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::GetGPUHandle() const
    {
        return static_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(mGpuHandle);
    }

    DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE&& handle) :
        mType(CPU)
    {
        mCpuHandle = static_cast<DescriptorHandle::CPUHandle>(handle.ptr);
    }

    DescriptorHandle::DescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE&& handle) :
        mType(GPU)
    {
        mGpuHandle = static_cast<DescriptorHandle::GPUHandle>(handle.ptr);
    }
}
