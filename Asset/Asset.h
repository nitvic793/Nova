#pragma once

#include <cstdint>
#include <Lib/Array.h>

namespace nv::asset
{
    enum AssetType : uint32_t
    {
        ASSET_INVALID = 0,
        ASSET_MESH,
        ASSET_SHADER,
        ASSET_TEXTURE
    };

    enum AssetChunkMarker : uint32_t
    {
        ASSET_CHUNK_START = 0x0,
        ASSET_CHUNK_END = 0xFFFFFFFF
    };

    struct AssetID
    {
        union 
        {
            struct 
            {
                uint32_t mType;
                uint32_t mHash;
            };

            uint64_t mId;
        };
    };

    struct Header
    {
        AssetID mAssetId;
        size_t  mSizeBytes;
        size_t  mOffset;
    };

    struct HeaderBlock
    {
        uint32_t    mHeaderSize;
        Header*     mpHeaders;
    };

    using AssetData         = Array<uint8_t>;
    using AssetDataArray    = Array<AssetData>;

    struct AssetChunk
    {
        HeaderBlock     mHeaderBlock;
        AssetDataArray  mAssetDataArray;
    };

    class Asset
    {
    public:
        constexpr AssetType GetType() const { return (AssetType)mId.mType; }
        constexpr uint32_t  GetHash() const { return mId.mHash; }
        constexpr uint64_t  GetID()   const { return mId.mId; }
        constexpr uint8_t*  GetData() const { return mData.mData; }
        constexpr size_t    Size()    const { return mData.mSize; }

    protected:
        AssetID     mId;
        AssetData   mData;
    };
}