#include "pch.h"
#include "TextureAsset.h"

#if NV_RENDERER_DX12
#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/ContextDX12.h>
#include <DX12/Interop.h>
#include <D3D12MemAlloc.h>
#include <wrl/client.h>
#include <DirectXTex.h>
#include <DDSTextureLoader.h>

#if _MSC_VER >= 1932 // Visual Studio 2022 version 17.2+
#    pragma comment(linker, "/alternatename:__imp___std_init_once_complete=__imp_InitOnceComplete")
#    pragma comment(linker, "/alternatename:__imp___std_init_once_begin_initialize=__imp_InitOnceBeginInitialize")
#endif

#endif

#include <Renderer/ResourceManager.h>
#include <Renderer/Renderer.h>

#include <Engine/Log.h>

namespace nv::asset
{
    void TextureAsset::Deserialize(const AssetData& data, graphics::Context* context)
    {
#if NV_RENDERER_DX12
        using namespace DirectX;
        using namespace Microsoft::WRL;
        using namespace graphics;

        auto device = (graphics::DeviceDX12*)graphics::gRenderer->GetDevice();
        auto renderer = (graphics::RendererDX12*)graphics::gRenderer;
        auto resHandle = graphics::gResourceManager->CreateEmptyResource();
        auto resource = (GPUResourceDX12*)graphics::gResourceManager->GetGPUResource(resHandle);
        auto ctx = (ContextDX12*)context;

        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        bool isCubeMap = false;
        auto hr = LoadDDSTextureFromMemoryEx(device->GetDevice(), data.mData, data.mSize, 0,
            D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_FLAGS::DDS_LOADER_DEFAULT,
            resource->GetResource().ReleaseAndGetAddressOf(), subresources, nullptr, &isCubeMap);

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource->GetResource().Get(), 0,
            static_cast<UINT>(subresources.size()));

        auto uploadResHandle = gResourceManager->CreateResource(GPUResourceDesc::UploadConstBuffer((uint32_t)uploadBufferSize));
        auto uploadRes = (GPUResourceDX12*)gResourceManager->GetGPUResource(uploadResHandle);

        resource = (GPUResourceDX12*)graphics::gResourceManager->GetGPUResource(resHandle); // Need to get resource again because if resource manager creates again, the pointer maybe invalid. 
        UpdateSubresources(ctx->GetCommandList(), resource->GetResource().Get(), uploadRes->GetResource().Get(),
            0, 0, static_cast<UINT>(subresources.size()), subresources.data());

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource->GetResource().Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx->GetCommandList()->ResourceBarrier(1, &barrier);
        gRenderer->QueueDestroy(uploadResHandle);
#endif

        mData.mBuffer = resHandle;
        mData.mFormat = GetFormat(resource->GetResource().Get()->GetDesc().Format);
        mData.mType = isCubeMap ? tex::TEXTURE_CUBE : tex::TEXTURE_2D;
        mData.mUsage = tex::USAGE_SHADER;
    }

    void TextureAsset::Export(const AssetData& data, std::ostream& ostream, Type type)
    {
        constexpr uint32_t MAX_MIP_LEVEL_GEN = 8;
#if NV_RENDERER_DX12
        using namespace DirectX;
        TexMetadata metadata = {};
        ScratchImage image;
        ScratchImage imageMipped;
        Blob blob;

        switch (type)
        {
        default:
        case WIC:
            LoadFromWICMemory(data.begin(), data.Size(), WIC_FLAGS_NONE, &metadata, image);
            break;
        case HDR:
            LoadFromHDRMemory(data.begin(), data.Size(), &metadata, image);
            break;
        case DDS:
            ostream.write((const char*)data.mData, data.mSize); // Just write through directly.
            break;
        }
        
        GenerateMipMaps(image.GetImages(), image.GetImageCount(), metadata, TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, MAX_MIP_LEVEL_GEN, imageMipped);
        SaveToDDSMemory(imageMipped.GetImages(), imageMipped.GetImageCount(), imageMipped.GetMetadata(), DDS_FLAGS_NONE, blob);
        ostream.write((const char*)blob.GetBufferPointer(), blob.GetBufferSize());
#endif
    }
}


