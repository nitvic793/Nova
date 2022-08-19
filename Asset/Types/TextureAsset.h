#pragma once

#include <Renderer/Texture.h>
#include <Asset.h>
#include <ostream>

namespace nv::asset
{
    class TextureAsset
    {
        using TextureDesc = graphics::TextureDesc;

    public:
        void Deserialize(const AssetData& data);
        void Export(const AssetData& data, std::ostream& ostream);

    private:
        TextureDesc mData = {};
    };
}