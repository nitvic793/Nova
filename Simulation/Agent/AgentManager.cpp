#include "pch.h"

#include "AgentManager.h"
#include <atomic>

namespace nv::sim
{
    using namespace agent;

    static std::atomic<uint64_t> sUIDCounter = 0;

    AgentManager::AgentManager():
        mAgentStore(nullptr),
        mActiveAgentCount(0)
    {
    }

    void AgentManager::Init()
    {
        mAgentStore = std::unique_ptr<IDataStore>(new AgentStore());
    }

    AgentID AgentManager::Spawn()
    {
        auto& store = mAgentStore->As<AgentStore>();
        const auto idx = mActiveAgentCount;
        mActiveAgentCount++;

        AgentStore::InstRef instance = store.Emplace(&GenerateUUID);

        return instance.Get<AgentID>();
    }

    uint64_t GenerateUUID()
    {
        sUIDCounter++;
        return sUIDCounter;
    }
}