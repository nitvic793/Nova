#pragma once

namespace nv::asset
{
    enum AssetType : uint32_t
    {
        ASSET_INVALID = 0,
        ASSET_MESH,
        ASSET_SHADER,
        ASSET_TEXTURE
    };

    struct AssetID
    {
        union 
        {
            struct 
            {
                uint32_t mType;
                uint32_t mIdentifier;
            };

            uint64_t mId;
        };
    };

    class Asset
    {

    };
}