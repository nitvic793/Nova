#pragma once

#include <Lib/Map.h>
#include <Lib/StringHash.h>
#include <Lib/Vector.h>

#include <Renderer/CommonDefines.h>
#include <Renderer/Renderer.h>

namespace nv::graphics
{
    struct ConstantBufferPoolData
    {
        nv::Vector<ConstantBufferView> mConstBufferViews;
        nv::Vector<ConstantBufferView> mFreeViews;
    };

    class ConstantBufferPool
    {
    public:
        template<typename T>
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

            if (data.mFreeViews.IsEmpty())
            {
                ConstantBufferView view = gRenderer->CreateConstantBuffer(sizeof(T));
                data.mFreeViews.Push(view);
            }

            auto view = data.mFreeViews.Pop();
            data.mConstBufferViews.Push(view);
            return view;
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