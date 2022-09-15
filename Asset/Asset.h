#pragma once

#include <cstdint>
#include <Lib/Array.h>
#include <Lib/Util.h>
#include <atomic>
#include <concepts>
#include <type_traits>
#include <AssetBase.h>

namespace nv::asset
{
    enum LoadState : uint8_t
    {
        STATE_UNLOADED,
        STATE_LOADING,
        STATE_LOADED,
        STATE_ERROR
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

    template <typename TSerializable>
    concept Serializable = std::is_member_pointer_v<decltype(&TSerializable::Deserialize)>;

    class Asset 
    {
    public:
        constexpr AssetType GetType()    const { return (AssetType)mId.mType; }
        constexpr uint32_t  GetHash()    const { return mId.mHash; }
        constexpr uint64_t  GetID()      const { return mId.mId; }
        constexpr uint8_t*  GetData()    const { return mData.mData; }
        constexpr size_t    Size()       const { return mData.mSize; }
        constexpr AssetID   GetAssetID() const { return mId; }

        constexpr const AssetData& GetAssetData() const { return mData; }

        constexpr void      SetData(const AssetData& data) { mData = data; }
        constexpr void      Set(AssetID id, const AssetData& data) { mId = id; mData = data; }
        constexpr void      SetBuffer(void* pBuffer, size_t size) { mData.mData = (uint8_t*)pBuffer; mData.mSize = size; }

        LoadState           GetState() const { return mState.load(); }
        void                SetState(LoadState state) { mState.store(state); }

        template<Serializable TSerializable, typename ...Args>
        constexpr void      DeserializeTo(TSerializable& type, Args&&... args)
        {
            type.Deserialize(mData, Forward<Args>(args)...);
        }

        template<Serializable TSerializable, typename ...Args>
        constexpr TSerializable DeserializeTo(Args&&... args)
        {
            TSerializable type;
            type.Deserialize(mData, Forward<Args>(args)...);
            return type;
        }

    protected:
        AssetID                 mId     = { (AssetType)0, 0 };
        AssetData               mData   = { 0, nullptr  };
        std::atomic<LoadState>  mState  = STATE_UNLOADED;
    };

    void Serialize(const Array<Asset>& assets, Array<uint8_t>& outBuffer, IAllocator* pAllocator = SystemAllocator::gPtr);
    void Deserialize(const Array<uint8_t>& buffer, Array<Asset>& outAssets, IAllocator* pAllocator = SystemAllocator::gPtr);
}