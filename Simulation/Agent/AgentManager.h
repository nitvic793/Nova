#pragma once

#include <Store/Store.h>
#include <Agent/Agent.h>

namespace nv::sim
{
    class AgentManager
    {
        static constexpr uint32_t INIT_AGENT_COUNT = 128;
    public:
        AgentManager();
        ~AgentManager() {}

        void Init();

        agent::AgentID Spawn();

    private:
        std::unique_ptr<IDataStore> mAgentStore;
        uint32_t                    mActiveAgentCount;
    };
}