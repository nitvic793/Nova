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
        mAgentStore->Resize(INIT_AGENT_COUNT);
    }

    AgentID AgentManager::Spawn()
    {
        auto& store = mAgentStore->As<AgentStoreType>();
        const auto idx = mActiveAgentCount;
        mActiveAgentCount++;
        if (mActiveAgentCount > store.GetSize())
        {
            store.Resize(store.GetSize() * 2);
        }

        AgentStoreType::InstRef instance = store.GetInstanceRef(idx);
        AgentID& id = instance.Get<AgentID>();
        id = (AgentID)GenerateUUID();

        instance.Get<AgentAge>() = (AgentAge)20;
        
        return id;
    }

    uint64_t GenerateUUID()
    {
        sUIDCounter++;
        return sUIDCounter;
    }
}