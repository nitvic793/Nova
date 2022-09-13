#pragma once

#include <cstdint>
#include <Lib/StringHash.h>

namespace nv::asset
{
    enum AssetType : uint32_t
    {
        ASSET_INVALID = 0,
        ASSET_MESH = ID("Mesh"),
        ASSET_SHADER = ID("Shader"),
        ASSET_TEXTURE = ID("Texture"),
        ASSET_CONFIG = ID("Config")
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
    };
}