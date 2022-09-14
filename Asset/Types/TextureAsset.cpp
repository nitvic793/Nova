#include "pch.h"
#include "TextureAsset.h"

#include <d3d12.h>
#include <DirectXTex.h>

namespace nv::asset
{
    void TextureAsset::Deserialize(const AssetData& data)
    {
        using namespace DirectX;
        TexMetadata info = {};
        ScratchImage image;
        LoadFromDDSMemory(data.mData, data.mSize, DDS_FLAGS_NONE, &info, image);
        // TODO: 
        // Load to GPU Resource
        // Store Metadata to Texture Desc
    }

    void TextureAsset::Export(const AssetData& data, std::ostream& ostream)
    {
        using namespace DirectX;
        TexMetadata metadata;
        ScratchImage image;
        Blob blob;

        LoadFromWICMemory(data.begin(), data.Size(), WIC_FLAGS_NONE, &metadata, image);
        SaveToDDSMemory(image.GetImages(), image.GetImageCount(), metadata, DDS_FLAGS_NONE, blob);
        ostream.write((const char*)blob.GetBufferPointer(), blob.GetBufferSize());
    }
}


