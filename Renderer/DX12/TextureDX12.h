#pragma once

#include <Lib/Handle.h>
#include <Renderer/Texture.h>

struct ID3D12Resource;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

namespace nv::graphics
{
    enum DescriptorViews : uint8_t
    {
        VIEW_NONE = 0,
        VIEW_RENDER_TARGET = 1,
        VIEW_SHADER_RESOURCE,
        VIEW_UNORDERED_ACCESS,
        VIEW_CONSTANT_BUFFER,
        VIEW_DEPTH_STENCIL,
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

        uint32_t        mHeapIndex  = 0;
        Type            mType       = NONE;
        DescriptorViews mView       = VIEW_NONE;

    public:
        DescriptorHandle() {}
        DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);
        DescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const;
    };

    class TextureDX12 : public Texture
    {
    public:
        TextureDX12() :
            Texture({}) {}
        TextureDX12(const TextureDesc& desc, const DescriptorHandle& handle) :
            Texture(desc),
            mDescriptorHandle(handle) {}
        
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const;
        ID3D12Resource*             GetResource() const;
        virtual uint32_t            GetHeapIndex() const override;
        virtual uint32_t            GetHeapOffset() const { return mDescriptorHandle.mHeapIndex; };

    private:
        DescriptorHandle mDescriptorHandle;
    };
}