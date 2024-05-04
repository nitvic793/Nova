#include "pch.h"

#include "Store.h"
#include "Agent/Agent.h"

namespace nv::sim
{
    void RegisterStores()
    {

    }

    void Store::Resize(size_t size)
    {
        for (auto& prop : mPropertyMap)
        {
            prop.second->Resize(size);
        }
    }
}