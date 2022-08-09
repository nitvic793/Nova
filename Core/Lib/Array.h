#pragma once

namespace nv
{
    template<typename TData>
    struct Array
    {
        size_t mSize;
        TData* mData;

        constexpr TData* begin()  const { return mData; }
        constexpr TData* end()    const { return mData + mSize; }

        constexpr TData* Size()   const { return mSize; }
        constexpr TData& operator[](size_t index) { return mData[index]; }
    };
}