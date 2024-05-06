#include "pch.h"

#include "AgentManager.h"

namespace nv::sim
{
    using namespace agent;

    void AgentManager::Init()
    {
        mAgentStore.Register<AgentUID>();
        mAgentStore.Register<Position>();
        mAgentStore.Register<AgentState>();
        mAgentStore.Register<AgentLocationState>();
        mAgentStore.Register<AgentAge>();

        mAgentStore.Resize(INIT_AGENT_COUNT);
    }
}