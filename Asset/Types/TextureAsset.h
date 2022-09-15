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
        void Deserialize(const AssetData& data, graphics::Context* context);
        void Export(const AssetData& data, std::ostream& ostream);
        constexpr const TextureDesc& GetDesc() const { return mData; }

    private:
        TextureDesc mData = {};
    };
}