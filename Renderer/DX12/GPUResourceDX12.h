#pragma once

#include <wrl/client.h>
#include <Renderer/GPUResource.h>

struct ID3D12Resource;

namespace nv::graphics
{
    class GPUResourceDX12 : public GPUResource
    {
    public:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        void SetResource(ID3D12Resource* pResource) noexcept;
        ComPtr<ID3D12Resource>& GetResource() noexcept;

    private:
        ComPtr<ID3D12Resource> mResource;
    };
}