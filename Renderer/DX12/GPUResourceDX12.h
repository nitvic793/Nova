#pragma once

#include <wrl/client.h>
#include <Renderer/GPUResource.h>
#include <Lib/Map.h>

struct ID3D12Resource;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

namespace nv::graphics
{
    enum DescriptorViews : uint8_t
    {
        VIEW_RENDER_TARGET = 0,
        VIEW_SHADER_RESOURCE,
        VIEW_UNORDERED_ACCESS,
        VIEW_CONSTANT_BUFFER,
        VIEWS_COUNT
    };

    struct DescriptorHandle
    {
        using CPUHandle = uint64_t;
        using GPUHandle = uint64_t;
        enum Type : uint8_t { NONE, CPU, GPU };

        union
        {
            CPUHandle mCpuHandle = 0;
            GPUHandle mGpuHandle;
        };

        Type mType = NONE;

    public:
        DescriptorHandle() {}
        DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE&& handle);
        DescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE&& handle);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const;
    };

    class GPUResourceDX12 : public GPUResource
    {
    public:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        void SetResource(ID3D12Resource* pResource) noexcept;
        ComPtr<ID3D12Resource>& GetResource() noexcept;
        DescriptorHandle        GetView(DescriptorViews view) const { return mViews[view]; }
        void                    SetView(DescriptorViews view, const DescriptorHandle& handle) { mViews[view] = handle; }

    private:
        ComPtr<ID3D12Resource>  mResource;
        DescriptorHandle        mViews[VIEWS_COUNT];
    };
}