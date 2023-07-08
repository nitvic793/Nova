#pragma once

#include <cstdint>
#include "Vector.h"
#include <Lib/Util.h>
#include <Lib/Pool.h>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>

namespace nv
{
    // Serializer for base engine types like Pool etc.
    class Serializer
    {
    public:
        template<typename T, typename TDerived>
        static void Serialize(Pool<T, TDerived>& pool, std::ostream& o)
        {
            using namespace cereal;
            cereal::JSONOutputArchive archive(o);
            archive(pool.mCapacity);
            archive(make_size_tag(static_cast<size_type>(pool.mSize)));
            archive(binary_data(pool.mBuffer, static_cast<std::size_t>(pool.mSize) * sizeof(TDerived)));
            archive(pool.mFreeIndices);
            archive(pool.mGenerations);
        }

        template<typename T, typename TDerived>
        static void Serialize(ContiguousPool<T, TDerived>& pool, std::ostream& o)
        {
            using namespace cereal;
            cereal::JSONOutputArchive archive(o);
            archive(pool.mCapacity);
            archive(pool.mPool);
            archive(pool.mHandleIndexMap);
            archive(pool.mGenerations);
        }

        template<typename T, typename TDerived>
        static void Deserialize(Pool<T, TDerived>& pool, std::istream& i)
        {
            using namespace cereal;
            cereal::JSONInputArchive archive(i);
            archive(pool.mCapacity);
            pool.GrowIfNeeded(pool.mCapacity);
            archive(make_size_tag(static_cast<size_type>(pool.mSize)));
            archive(binary_data(pool.mBuffer, static_cast<std::size_t>(pool.mSize) * sizeof(TDerived)));
            archive(pool.mFreeIndices);
            archive(pool.mGenerations);
        }

        template<typename T, typename TDerived>
        static void Deserialize(ContiguousPool<T, TDerived>& pool, std::istream& i)
        {
            using namespace cereal;
            cereal::JSONInputArchive archive(i);
            archive(pool.mCapacity);
            archive(pool.mPool);
            archive(pool.mHandleIndexMap);
            archive(pool.mGenerations);
        }
    };
}