#pragma once

#include <wrl/client.h>
#include <Renderer/GPUResource.h>
#include <Lib/Map.h>

struct ID3D12Resource;

namespace nv::graphics
{
    class GPUResourceDX12 : public GPUResource
    {
    public:
        GPUResourceDX12() : 
            GPUResource({}) {}
        GPUResourceDX12(const GPUResourceDesc& desc) :
            GPUResource(desc) {}

        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        void                    SetResource(ID3D12Resource* pResource) noexcept;
        ComPtr<ID3D12Resource>& GetResource() noexcept;

    private:
        ComPtr<ID3D12Resource>  mResource;
    };
}