#include "pch.h"
#include "TextureAsset.h"

#include <d3d12.h>
#include <DirectXTex.h>

namespace nv::asset
{
    void TextureAsset::Deserialize(const AssetData& data)
    {
    }

    void TextureAsset::Export(const AssetData& data, std::ostream& ostream)
    {
        using namespace DirectX;
        TexMetadata metadata;
        ScratchImage image;
        
        LoadFromWICMemory(data.begin(), data.Size(), WIC_FLAGS_NONE, &metadata, image);
        Blob blob;
        if (metadata.arraySize == 1)
        {
            //SaveToDDSMemory(, DDS_FLAGS_NONE, blob);
        }
        
    }
}


