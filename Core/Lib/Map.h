#ifndef NV_MAP
#define NV_MAP

#pragma once

#include <ska/flat_hash_map.hpp>
#include <map>

namespace nv
{
    template<typename K, typename V>
    using HashMap = ska::flat_hash_map<K, V>;

    template<typename K, typename V>
    using OrderedMap = std::map<K, V>;

    template<typename V>
    using HashSet = ska::flat_hash_set<V>;
}

#endif // !NV_MAP
