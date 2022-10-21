#pragma once

#include <wrl/client.h>
#include <Renderer/GPUResource.h>
#include <Lib/Map.h>

struct ID3D12Resource;
struct ID3D12GraphicsCommandList;

namespace D3D12MA
{
    class Allocation;
}

namespace nv::graphics
{
    struct UploadData
    {
        const void* mpData = nullptr;
        int64_t     mRowPitch = 0;
        int64_t     mSlicePitch = 0;

        uint64_t    mIntermediateOffset = 0;
        uint32_t    mFirstSubresource = 0;
        uint32_t    mNumSubResources = 1;
    };

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
        void                    SetResource(D3D12MA::Allocation* pAllocation, ID3D12Resource* pResource = nullptr) noexcept;
        ComPtr<ID3D12Resource>& GetResource() noexcept;

        void                    UploadResource(ID3D12GraphicsCommandList* pCmdList, const UploadData& data, ID3D12Resource* pIntermediate);

        // Inherited via GPUResource
        virtual void            MapMemory();
        virtual void            UnmapMemory();
        virtual void            UploadMapped(uint8_t* bytes, size_t size, size_t offset = 0Ui64);

    private:
        ComPtr<ID3D12Resource>      mResource;
        ComPtr<D3D12MA::Allocation> mAllocation;


    };
}