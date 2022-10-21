#include "pch.h"

#include "GPUResourceDX12.h"
#include "DirectXIncludes.h"
#include <D3D12MemAlloc.h>

using namespace Microsoft::WRL;

namespace nv::graphics
{
    void GPUResourceDX12::SetResource(ID3D12Resource* pResource) noexcept
    {
        mResource.Attach(pResource);
    }

    void GPUResourceDX12::SetResource(D3D12MA::Allocation* pAllocation, ID3D12Resource* pResource) noexcept
    {
        mAllocation.Attach(pAllocation);
        if (pResource)
        {
            mResource.Attach(pResource);
        }
        else if(!mResource)
        {
            mResource.Attach(mAllocation->GetResource());
            mResource->AddRef();
        }
    }

    ComPtr<ID3D12Resource>& GPUResourceDX12::GetResource() noexcept
    {
        return mResource;
    }

    void GPUResourceDX12::UploadResource(ID3D12GraphicsCommandList* pCmdList, const UploadData& data, ID3D12Resource* pIntermediate)
    {
        const D3D12_SUBRESOURCE_DATA subData =
        {
            .pData = data.mpData,
            .RowPitch = data.mRowPitch,
            .SlicePitch = data.mSlicePitch
        };

        ::UpdateSubresources(pCmdList, mResource.Get(), pIntermediate, data.mIntermediateOffset, data.mFirstSubresource, data.mNumSubResources, &subData);
    }

    void GPUResourceDX12::MapMemory()
    {
        CD3DX12_RANGE readRange(0, 0);
        mResource->Map(0, &readRange, reinterpret_cast<void**>(&mMappedAddress));
    }

    void GPUResourceDX12::UnmapMemory()
    {
        mResource->Unmap(0, nullptr);
        mMappedAddress = nullptr;
    }

    void GPUResourceDX12::UploadMapped(uint8_t* bytes, size_t size, size_t offset)
    {
        assert(mMappedAddress);
        char* ptr = reinterpret_cast<char*>(mMappedAddress) + offset;
        memcpy(ptr, bytes, size);
    }
}
