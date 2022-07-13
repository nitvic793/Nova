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

}
