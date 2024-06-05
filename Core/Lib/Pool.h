#pragma once

#include <cstdint>
#include "Vector.h"
#include <Lib/Util.h>
#include <Lib/Map.h>

namespace nv
{
    constexpr uint32_t kPoolInitDefaultSize = 4;

    template<typename T, typename TDerived = T, uint32_t InitPoolCount = kPoolInitDefaultSize>
    class Pool
    {
    public:
        static constexpr uint32_t kDefaultPoolCount = InitPoolCount;

    public:
        Pool() :
            mBuffer(nullptr),
            mSize(0),
            mCapacity(kDefaultPoolCount),
            mGenerations(kDefaultPoolCount)
        {
            mGenerations.resize(kDefaultPoolCount);
            mFreeIndices.reserve(kDefaultPoolCount);
        }

        void Init()
        {
            mBuffer = (TDerived*)SystemAllocator::gPtr->Allocate(sizeof(TDerived) * kDefaultPoolCount);
            mGenerations.resize(kDefaultPoolCount);
            mFreeIndices.reserve(kDefaultPoolCount);
            memset(mGenerations.data(), 0, mGenerations.size() * sizeof(uint32_t));
        }

        void Destroy()
        {
            for (auto i = 0u; i < mSize; ++i)
            {
                if (std::find(mFreeIndices.begin(), mFreeIndices.end(), i) == std::end(mFreeIndices))
                {
                    GetIndex(i)->~TDerived();
                }
            }

            Clear();
        }

        ~Pool() 
        {
            if (mBuffer)
                SystemAllocator::gPtr->Free(mBuffer);
            mBuffer = nullptr;
        }

    public:
        template<typename ...Args>
        Handle<T> Create(Args&&... args)
        {
            Handle<T> handle;
            T* data = nullptr;
            assert(mBuffer != nullptr);
            if (mFreeIndices.size() == 0)
            {
                GrowIfNeeded();
                data = GetIndex(mSize);
                handle.mIndex = mSize;
                mGenerations[handle.mIndex] = 1;
                mSize++;
            }
            else
            {
                handle.mIndex = mFreeIndices.back();
                mFreeIndices.pop_back();
                data = GetIndex(handle);
            }

            new (data) TDerived(nv::Forward<Args>(args)...); // Placement construct
            handle.mGeneration = mGenerations[handle.mIndex];
            return handle;
        }

        template<typename ...Args>
        TDerived* CreateInstance(Handle<T>& outHandle, Args&&... args)
        {
            outHandle = Create(Forward<Args>(args)...);
            return GetIndex(outHandle);
        }

        constexpr T* Get(Handle<T> handle) const
        {
            if (!IsValid(handle))
                return nullptr;
            return GetIndex(handle);
        }

        constexpr TDerived* GetAsDerived(Handle<T> handle) const
        {
            if (!IsValid(handle))
                return nullptr;
            return (TDerived*)GetIndex(handle);
        }

        constexpr bool IsValid(Handle<T> handle) const
        {
            return handle.mGeneration == mGenerations[handle.mIndex];
        }

        void Remove(Handle<T> handle)
        {
            if (IsValid(handle))
            {
                mGenerations[handle.mIndex]++;
                mFreeIndices.push_back(handle.mIndex);
                GetIndex(handle)->~TDerived();
            }
        }

        Handle<T> Insert(const TDerived& data)
        {
            return Create(data);
        }

        Handle<T> Insert(TDerived&& data)
        {
            return Create(data);
        }

        constexpr Span<TDerived> Slice(size_t start, size_t end) const
        {
            return Span().Slice(start, end);
        }

        constexpr Span<TDerived> Span() const
        {
            return ::nv::Span<TDerived>{ (TDerived*)mBuffer, mSize };
        }

        void GetAllHandles(Vector<Handle<T>>& outHandles) const
        {
            for (uint32_t i = 0; i < mSize; ++i)
            {
                const uint32_t gen = mGenerations[i];
                const Handle<T> handle(i, gen);
                if (IsValid(handle))
                {
                    outHandles.Push(handle);
                }
            }
        }

        constexpr void Clear()
        {
            mFreeIndices.clear();
            mGenerations.clear();
            mGenerations.resize(mCapacity);
            mSize = 0;
        }

        constexpr TDerived* GetIndex(uint32_t index) const
        {
            return (TDerived*)((Byte*)mBuffer + index * sizeof(TDerived));
        }

        template<typename TFunc>
        void ForEach(TFunc&& func)
        {
            for (uint32_t i = 0; i < mSize; ++i)
            {
                const Handle<T> handle(i, mGenerations[i]);
                if (IsValid(handle))
                {
                    func(GetIndex(handle));
                }
            }
        }

        constexpr bool      IsEmpty() const { return mFreeIndices.size() == mSize; }
        constexpr size_t    GetStrideSize() const { return sizeof(TDerived); }
        constexpr uint32_t  Size() const { return mSize; }
        constexpr uint32_t  Capacity() const { return mCapacity; }
        constexpr TDerived* Data() const { return mBuffer; }

    private:
        void GrowIfNeeded()
        {
            GrowIfNeeded(mSize + 1);
        }

        void GrowIfNeeded(size_t requestSize)
        {
            if (requestSize > mCapacity)
            {
                auto oldCapacity = mCapacity;
                mCapacity *= 2;
                mCapacity = mCapacity >= requestSize ? mCapacity : (uint32_t)requestSize;
                void* pBuffer = SystemAllocator::gPtr->Allocate(sizeof(TDerived) * mCapacity);// , mBuffer);
                memcpy(pBuffer, mBuffer, oldCapacity * sizeof(TDerived));
                if (mBuffer)
                    SystemAllocator::gPtr->Free(mBuffer);
                mBuffer = (TDerived*)pBuffer;

                mGenerations.resize(mCapacity);
            }
        }

        constexpr TDerived* GetIndex(Handle<T> handle) const
        {
            return GetIndex(handle.mIndex);
        }

    private:
        TDerived*               mBuffer;
        uint32_t                mSize;
        uint32_t                mCapacity;
        std::vector<uint32_t>   mFreeIndices;
        std::vector<uint32_t>   mGenerations;

        friend class Serializer;
    };

    // Gauranteed to have elements contiguously in memory. Useful for iterating.
    // More expensive to look up handle since it uses a map internally. 
    template<typename T, typename TDerived = T, uint32_t InitPoolCount = kPoolInitDefaultSize>
    class ContiguousPool
    {
    public:
        ContiguousPool() :
            mCapacity(InitPoolCount)
        {
            mGenerations.resize(InitPoolCount);
            mPool.reserve(InitPoolCount);
            for (auto& val : mGenerations)
                val = 0;
        }

        void Init() {}
        void Destroy() {}

        template<typename ...Args>
        Handle<T> Create(Args&&... args)
        {
            Handle<T> handle;
            GrowIfNeeded();
            handle.mIndex = (uint32_t)mPool.size();
            mPool.emplace_back(nv::Forward<Args>(args)...);
            
            handle.mGeneration = mGenerations[handle.mIndex];
            mHandleIndexMap[handle.mHandle] = handle.mIndex;

            return handle;
        }

        Handle<T> Insert(const TDerived& data)
        {
            return Create(data);
        }

        Handle<T> Insert(TDerived&& data)
        {
            return Create(data);
        }

        constexpr Span<TDerived> Span() const
        {
            auto pData = mPool.data();
            auto pTData = (TDerived*)pData;
            return nv::Span<TDerived>(pTData, mPool.size());
        }

        constexpr bool IsValid(Handle<T> handle) const
        {
            auto it = mHandleIndexMap.find(handle.mHandle);
            if (it == mHandleIndexMap.end())
                return false;

            const uint32_t idx = it->second;
            return handle.mGeneration == mGenerations[idx];
        }

        constexpr T* Get(Handle<T> handle) const
        {
            const uint32_t idx = mHandleIndexMap.at(handle.mHandle);
            return (T*)&mPool[idx];
        }

        constexpr const TDerived* GetAsDerived(Handle<T> handle) const
        {
            const uint32_t idx = mHandleIndexMap.at(handle.mHandle);
            return &mPool.at(idx);
        }

        constexpr TDerived* GetAsDerived(Handle<T> handle)
        {
            const uint32_t idx = mHandleIndexMap.at(handle.mHandle);
            return &mPool[idx];
        }

        constexpr size_t Size() const
        {
            return mPool.size();
        }

        constexpr TDerived& operator[](size_t idx) { return mPool[idx]; }
        constexpr const TDerived& operator[](size_t idx) const { return mPool.at(idx); }

        void Remove(Handle<T> handle)
        {
            if (!IsValid(handle))
                return;

            const uint32_t idx = mHandleIndexMap.at(handle.mHandle);
            const uint32_t lastIdx = (uint32_t)mPool.size() - 1;
            const auto lastHandle = Handle<T>(lastIdx, mGenerations[lastIdx]);

            std::swap(mPool[idx], mPool[lastIdx]);
            std::swap(mGenerations[idx], mGenerations[lastIdx]);
            TDerived& val = mPool.back();
            mPool.pop_back();

            val.~TDerived();
            mGenerations[lastIdx]++;
            
            mHandleIndexMap[lastHandle.mHandle] = idx;
            mHandleIndexMap.erase(handle.mHandle);
        }

        void CopyToPool(TDerived* pData, size_t count)
        {
            mPool.resize(count/*, true*/);
            memcpy(mPool.data(), pData, count * sizeof(TDerived));
        }

    private:
        void GrowIfNeeded(size_t requestedSize)
        {
            if (requestedSize >= mCapacity)
            {
                mCapacity = mCapacity * 2;
                mCapacity = mCapacity >= requestedSize ? mCapacity : (uint32_t)requestedSize;
                mGenerations.resize(mCapacity/*, true*/);
                for (uint32_t i = (uint32_t)mPool.size(); i < mCapacity; ++i)
                    mGenerations[i] = 0;
            }
        }

        void GrowIfNeeded()
        {
            GrowIfNeeded(mPool.size() + 1);
        }

    private:
        uint32_t                mCapacity;
        std::vector<TDerived>    mPool;
        std::vector<uint32_t>    mGenerations;

        UnorderedMap<uint64_t, uint32_t> mHandleIndexMap;

        friend class Serializer;
    };
}
