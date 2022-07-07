#pragma once

#include <cstdint>
#include "Vector.h"

namespace nv
{
    constexpr uint32_t kPoolInitDefaultSize = 4;

    template<typename T, typename TDerived = T, uint32_t InitPoolCount = kPoolInitDefaultSize>
    class Pool
    {
    public:
        static constexpr uint32_t   kDefaultPoolCount = InitPoolCount;

    public:
        Pool() :
            mData(kDefaultPoolCount),
            mFreeIndices(kDefaultPoolCount),
            mGenerations(kDefaultPoolCount)
        {
            mGenerations.SetSize(kDefaultPoolCount);
        }

        ~Pool() {}

    public:
        template<typename ...Args>
        Handle<T> Create(Args&&... args)
        {
            Handle<T> handle;
            T* data = nullptr;
            if (mFreeIndices.Size() == 0)
            {
                data = &mData.Emplace();
                handle.mIndex = (uint32_t)mData.Size() - 1;
                mGenerations[handle.mIndex] = 1;
            }
            else
            {
                handle.mIndex = mFreeIndices.Pop();
                data = &mData[handle.mIndex];
            }

            new (data) TDerived(std::forward<Args>(args)...); //Placement construct
            handle.mGeneration = mGenerations[handle.mIndex];
            return handle;
        }

        constexpr T* Get(Handle<T> handle) const
        {
            if (!IsValid(handle))
                return nullptr;
            return &mData[handle.mIndex];
        }

        constexpr TDerived* GetAsDerived(Handle<T> handle) const
        {
            if (!IsValid(handle))
                return nullptr;
            return &mData[handle.mIndex];
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
                mData[handle.mIndex].~TDerived();
            }
        }

        Handle<T> Insert(const TDerived& data)
        {
            return Create(data);

            //Handle<T> handle;
            //if (mFreeIndices.Size() == 0)
            //{
            //    mData.Push(data);
            //    handle.mIndex = (uint32_t)mData.Size() - 1;
            //    mGenerations[handle.mIndex] = 1;
            //}
            //else
            //{
            //    handle.mIndex = mFreeIndices.Pop();
            //    mData[handle.mIndex] = std::move(data);
            //}

            //handle.mGeneration = mGenerations[handle.mIndex];
            //return handle;
        }

        Handle<T> Insert(TDerived&& data)
        {
            return Create(data);
        }

        constexpr Span<TDerived> Slice(size_t start, size_t end) const
        {
            return mData.Slice(start, end);
        }

        constexpr Span<TDerived> Span() const
        {
            return mData.Span();
        }

        constexpr void Clear()
        {
            mData.Clear();
            mFreeIndices.Clear();
            mGenerations.Clear();
        }

    private:
        nv::Vector<TDerived>    mData;
        nv::Vector<uint32_t>    mFreeIndices;
        nv::Vector<uint32_t>    mGenerations;
    };
}
