#pragma once

#include <Renderer/Texture.h>
#include <Renderer/Context.h>
#include <Asset.h>
#include <ostream>

namespace nv::asset
{
    class TextureAsset
    {
        using TextureDesc = graphics::TextureDesc;

    public:
        enum Type
        {
            WIC, DDS, HDR
        };

    public:
        void Deserialize(const AssetData& data, graphics::Context* context);
        void Export(const AssetData& data, std::ostream& ostream, Type type = WIC);
        constexpr const TextureDesc& GetDesc() const { return mData; }

    private:
        TextureDesc mData = {};
    };
}