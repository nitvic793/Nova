#pragma once

#include <cstdint>
#include <Lib/Array.h>

namespace nv::asset
{
    enum AssetType : uint32_t
    {
        ASSET_INVALID   = 0,
        ASSET_MESH      = ID("Mesh"),
        ASSET_SHADER    = ID("Shader"),
        ASSET_TEXTURE   = ID("Texture")
    };

    enum LoadState : uint8_t
    {
        STATE_UNLOADED,
        STATE_LOADING,
        STATE_LOADED,
        STATE_ERROR
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

        constexpr operator uint64_t() const { return mId; }
    };

    struct Header
    {
        AssetID mAssetId;
        size_t  mSizeBytes;
        size_t  mOffset;
    };

    using HeaderBlock = Array<Header>;
    using AssetData = Array<uint8_t>;
    using AssetDataArray = Array<AssetData>;

    struct AssetChunk
    {
        HeaderBlock     mHeaderBlock;
        AssetDataArray  mAssetDataArray;
    };

    template <typename T>
    concept Serializable =
        requires(T& t, AssetData& data) 
         {
             { t.Deserialize(data) } -> std::same_as<void>;
         };

    class Asset 
    {
    public:
        constexpr AssetType GetType()    const { return (AssetType)mId.mType; }
        constexpr uint32_t  GetHash()    const { return mId.mHash; }
        constexpr uint64_t  GetID()      const { return mId.mId; }
        constexpr uint8_t*  GetData()    const { return mData.mData; }
        constexpr size_t    Size()       const { return mData.mSize; }
        constexpr LoadState GetState()   const { return mState; }
        constexpr AssetID   GetAssetID() const { return mId; }

        constexpr const AssetData& GetAssetData() const { return mData; }

        constexpr void      SetData(const AssetData& data) { mData = data; }
        constexpr void      Set(AssetID id, const AssetData& data) { mId = id; mData = data; }
        constexpr void      SetState(LoadState state) { mState = state; }
        constexpr void      SetBuffer(void* pBuffer, size_t size) { mData.mData = (uint8_t*)pBuffer; mData.mSize = size; }


        template<Serializable TSerializable>
        constexpr void      DeserializeTo(TSerializable& type)
        {
            type.Deserialize(mData);
        }

        template<Serializable TSerializable>
        constexpr TSerializable DeserializeTo()
        {
            TSerializable type;
            type.Deserialize(mData);
            return type;
        }

    protected:
        AssetID     mId     = { 0 };
        AssetData   mData   = { 0, nullptr  };
        LoadState   mState  = STATE_UNLOADED;
    };

    void Serialize(const Array<Asset>& assets, Array<uint8_t>& outBuffer, IAllocator* pAllocator = SystemAllocator::gPtr);
    void Deserialize(const Array<uint8_t>& buffer, Array<Asset>& outAssets, IAllocator* pAllocator = SystemAllocator::gPtr);
}