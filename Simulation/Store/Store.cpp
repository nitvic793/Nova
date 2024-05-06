#include "pch.h"

#include "Store.h"
#include "Agent/Agent.h"

namespace nv::sim
{
    void Store::Resize(size_t size)
    {
        for (auto& prop : mPropertyMap)
        {
            prop.second->Resize(size);
        }
    }

    void Store::Swap(uint32_t idxA, uint32_t idxB)
    {
        for (auto& prop : mPropertyMap)
        {
            prop.second->Swap(idxA, idxB);
        }
    }

    size_t Store::Size() const
    {
        return mPropertyMap.begin()->second->Size();
    }
}