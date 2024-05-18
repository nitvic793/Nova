#include "pch.h"

#include "Store.h"
#include "Agent/Agent.h"

namespace nv::sim
{
    static std::atomic<uint64_t> sUIDCounter = 0;
    DataStoreFactory sgDataStoreFactory;

    static_assert(std::atomic<uint64_t>::is_always_lock_free, "Atomic uint64_t is not lock free");

    uint64_t GenerateUUID()
    {
        sUIDCounter++;
        return sUIDCounter;
    }
}