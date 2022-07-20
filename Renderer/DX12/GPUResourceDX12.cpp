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
}
