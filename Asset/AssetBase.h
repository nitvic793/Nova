#pragma once

#include <cstdint>
#include <Lib/StringHash.h>

#define NV_SERIALIZE(...) template<class Archive> void serialize(Archive& archive) { archive(##__VA_ARGS__); }

namespace nv::asset
{
    enum AssetType : uint32_t
    {
        ASSET_INVALID   = 0,
        ASSET_MESH      = ID("Mesh"),
        ASSET_SHADER    = ID("Shader"),
        ASSET_TEXTURE   = ID("Texture"),
        ASSET_CONFIG    = ID("Config"),
        ASSET_DB        = ID("DB")
    };

    struct AssetID
    {
        union
        {
            struct
            {
                AssetType   mType;
                uint32_t    mHash;
            };

            uint64_t mId;
        };

        constexpr operator uint64_t() const { return mId; }
        constexpr AssetID& operator=(uint64_t id) { mId = id; return *this; }

        NV_SERIALIZE(mId);
    };

    struct IDesc
    {
        AssetID mAssetId;
    };
}