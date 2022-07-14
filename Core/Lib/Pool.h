#pragma once

#include <cstdint>
#include "Vector.h"
#include <Lib/Util.h>

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
            mBuffer = (T*)SystemAllocator::gPtr->Allocate(sizeof(TDerived) * kDefaultPoolCount);
        }

        void Destroy()
        {
            for (auto i = 0u; i < mSize; ++i)
            {
                if (!mFreeIndices.Exists(i))
                {
                    GetIndex(i)->~T();
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
        T* CreateInstance(Handle<T>& outHandle, Args&&... args)
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
                GetIndex(handle)->~T();
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

        constexpr void Clear()
        {
            mFreeIndices.Clear();
            mGenerations.Clear();
            mSize = 0;
        }

    private:
        void GrowIfNeeded()
        {
            if (mSize + 1 >= mCapacity)
            {
                auto oldCapacity = mCapacity;
                mCapacity *= 2;
                void* pBuffer = SystemAllocator::gPtr->Allocate(sizeof(TDerived) * mCapacity);
                memcpy(pBuffer, mBuffer, oldCapacity * sizeof(TDerived));
                if(mBuffer)
                    SystemAllocator::gPtr->Free(mBuffer);
                mBuffer = (T*)pBuffer;
            }
        }

        constexpr T* GetIndex(Handle<T> handle) const
        {
            return GetIndex(handle.mIndex);
        }

        constexpr T* GetIndex(uint32_t index) const
        {
            return (T*)((Byte*)mBuffer + index * sizeof(TDerived));
        }

    private:
        T*                      mBuffer;
        uint32_t                mSize;
        uint32_t                mCapacity;
        nv::Vector<uint32_t>    mFreeIndices;
        nv::Vector<uint32_t>    mGenerations;
    };
}
