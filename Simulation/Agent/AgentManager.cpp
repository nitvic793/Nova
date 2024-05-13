#include "pch.h"

#include "AgentManager.h"
#include <atomic>

namespace nv::sim
{
    using namespace agent;

    static std::atomic<uint64_t> sUIDCounter = 0;

    using AgentStoreType = ArchetypeStore<AgentArchetype>;

    AgentManager::AgentManager():
        mAgentStore(nullptr),
        mActiveAgentCount(0)
    {
    }

    void AgentManager::Init()
    {
        mAgentStore = std::unique_ptr<IDataStore>(new ArchetypeStore<AgentArchetype>());
    }

    AgentID AgentManager::Spawn()
    {
        auto& store = mAgentStore->As<AgentStoreType>();
        const auto idx = mActiveAgentCount;
        mActiveAgentCount++;

        AgentStoreType::InstRef instance = store.Emplace(&GenerateUUID);

        return instance.Get<AgentID>();
    }

    uint64_t GenerateUUID()
    {
        sUIDCounter++;
        return sUIDCounter;
    }
}