#pragma once

#include <Lib/Map.h>
#include <Lib/StringHash.h>
#include <Lib/Vector.h>

#include <Renderer/CommonDefines.h>
#include <Renderer/Renderer.h>

#include <atomic>

namespace nv::graphics
{
    constexpr size_t MAX_RING_BUFFER_SIZE = 1024;

    template<typename T>
    class RingBuffer
    {
    public:
        RingBuffer() :
            mCurrentIndex(0)
        {}

        void Insert(const T& item)
        {
            mData.Push(item);
        }

        T* Get() const
        {
            assert(mCurrentIndex < mData.Size());
            T* val = &mData[mCurrentIndex];

            mCurrentIndex++;
            if (mCurrentIndex >= mData.Size())
                mCurrentIndex = 0;
            return val;
        }

        constexpr bool IsFull() const { return mCurrentIndex == mData.Size() - 1; }
        constexpr size_t Size() const { return mData.Size(); }

    private:
        nv::Vector<T>	mData;
        mutable size_t	mCurrentIndex;
    };

    struct ConstantBufferPoolData
    {
        nv::Vector<ConstantBufferView>  mConstBufferViews;
        nv::Vector<ConstantBufferView>  mFreeViews;
        RingBuffer<ConstantBufferView> mRingBuffer;
    };

    class ConstantBufferPool
    {
    public:
        template<typename T, size_t TMaxSize = MAX_RING_BUFFER_SIZE>
        ConstantBufferView GetConstantBuffer()
        {
            constexpr StringID typeId = TypeNameID<T>();
            auto it = mConstBufferPoolMap.find(typeId);
            if (it == mConstBufferPoolMap.end())
            {
                mConstBufferPoolMap[typeId] = ConstantBufferPoolData{};
                it = mConstBufferPoolMap.find(typeId);
            }

            ConstantBufferPoolData& data = it->second;
            size_t size = data.mRingBuffer.Size();
            size_t maxSize = TMaxSize;
            if (size < maxSize) // Fill up buffer upto max size
            {
                ConstantBufferView view = gRenderer->CreateConstantBuffer(sizeof(T));
                data.mRingBuffer.Insert(view);
                return view;
            }
            else // Once full, use ring buffer logic to get next available item.
            {
                return *data.mRingBuffer.Get();
            }

            //if (data.mFreeViews.IsEmpty())
            //{
            //    ConstantBufferView view = gRenderer->CreateConstantBuffer(sizeof(T));
            //    data.mFreeViews.Push(view);
            //}

            //auto view = data.mFreeViews.Pop();
            //data.mConstBufferViews.Push(view);
            //return view;
        }

        template<typename T>
        void Clear()
        {
            constexpr StringID typeId = TypeNameID<T>();

            auto it = mConstBufferPoolMap.find(typeId);
            ConstantBufferPoolData& data = it->second;

            while (!data.mConstBufferViews.IsEmpty())
            {
                auto view = data.mConstBufferViews.Pop();
                data.mFreeViews.Push(view);
            }
        }

        void Clear()
        {
            for (auto& it : mConstBufferPoolMap)
            {
                ConstantBufferPoolData& data = it.second;
                while (!data.mConstBufferViews.IsEmpty())
                {
                    auto view = data.mConstBufferViews.Pop();
                    data.mFreeViews.Push(view);
                }
            }
        }

    private:
        HashMap<StringID, ConstantBufferPoolData> mConstBufferPoolMap;
    };

}