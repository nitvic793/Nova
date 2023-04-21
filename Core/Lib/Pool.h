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
            mFreeIndices(kDefaultPoolCount),
            mGenerations(kDefaultPoolCount)
        {
            mGenerations.SetSize(kDefaultPoolCount);
        }

        void Init()
        {
            mBuffer = (TDerived*)SystemAllocator::gPtr->Allocate(sizeof(TDerived) * kDefaultPoolCount);
        }

        void Destroy()
        {
            for (auto i = 0u; i < mSize; ++i)
            {
                if (!mFreeIndices.Exists(i))
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
            if (mFreeIndices.Size() == 0)
            {
                GrowIfNeeded();
                data = GetIndex(mSize);
                handle.mIndex = mSize;
                mGenerations[handle.mIndex] = 1;
                mSize++;
            }
            else
            {
                handle.mIndex = mFreeIndices.Pop();
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
                mFreeIndices.Push(handle.mIndex);
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
            mFreeIndices.Clear();
            mGenerations.Clear();
            mSize = 0;
        }

        constexpr TDerived* GetIndex(uint32_t index) const
        {
            return (TDerived*)((Byte*)mBuffer + index * sizeof(TDerived));
        }

        constexpr bool      IsEmpty() const { return mFreeIndices.Size() == mSize; }
        constexpr size_t    GetStrideSize() const { return sizeof(TDerived); }
        constexpr uint32_t  Size() const { return mSize; }
        constexpr uint32_t  Capacity() const { return mCapacity; }
        constexpr TDerived* Data() const { return mBuffer; }

    private:
        void GrowIfNeeded()
        {
            if (mSize + 1 > mCapacity)
            {
                auto oldCapacity = mCapacity;
                mCapacity *= 2;
                void* pBuffer = SystemAllocator::gPtr->Allocate(sizeof(TDerived) * mCapacity);// , mBuffer);
                memcpy(pBuffer, mBuffer, oldCapacity * sizeof(TDerived));
                if(mBuffer)
                    SystemAllocator::gPtr->Free(mBuffer);
                mBuffer = (TDerived*)pBuffer;

                mGenerations.Grow(mCapacity, true);
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
        nv::Vector<uint32_t>    mFreeIndices;
        nv::Vector<uint32_t>    mGenerations;
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
            mGenerations.SetSize(InitPoolCount);
            mPool.Reserve(InitPoolCount);
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
            handle.mIndex = (uint32_t)mPool.Size();
            mPool.Emplace(nv::Forward<Args>(args)...);
            
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
            return mPool.Span();
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

        constexpr TDerived* GetAsDerived(Handle<T> handle) const
        {
            const uint32_t idx = mHandleIndexMap.at(handle.mHandle);
            return &mPool[idx];
        }

        constexpr size_t Size() const
        {
            return mPool.Size();
        }

        constexpr TDerived& operator[](size_t idx) const { return mPool[idx]; }

        void Remove(Handle<T> handle)
        {
            if (!IsValid(handle))
                return;

            const uint32_t idx = mHandleIndexMap.at(handle.mHandle);
            const uint32_t lastIdx = (uint32_t)mPool.Size() - 1;
            const auto lastHandle = Handle<T>(lastIdx, mGenerations[lastIdx]);

            std::swap(mPool[idx], mPool[lastIdx]);
            std::swap(mGenerations[idx], mGenerations[lastIdx]);
            TDerived& val = mPool.Pop();
            val.~TDerived();
            mGenerations[lastIdx]++;
            
            mHandleIndexMap[lastHandle.mHandle] = idx;
            mHandleIndexMap.erase(handle.mHandle);
        }

    private:
        void GrowIfNeeded()
        {
            if (mPool.Size() + 1 >= mCapacity)
            {
                mCapacity = mCapacity * 2;
                mGenerations.Grow(mCapacity, true);
                for (uint32_t i = (uint32_t)mPool.Size(); i < mCapacity; ++i)
                    mGenerations[i] = 0;
            }
        }

    private:
        uint32_t                mCapacity;
        nv::Vector<TDerived>    mPool;
        nv::Vector<uint32_t>    mGenerations;

        HashMap<uint64_t, uint32_t> mHandleIndexMap;
    };
}
