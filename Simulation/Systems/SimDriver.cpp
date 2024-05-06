#include "pch.h"

#include "SimDriver.h"
#include <Agent/Agent.h>
#include <Engine/JobSystem.h>


namespace nv
{
    using namespace sim::agent;

    void SimDriver::Init()
    {
        jobs::InitJobSystem(4);
        
        mAgentStore.Register<AgentUID>();
        mAgentStore.Register<Position>();
        mAgentStore.Register<AgentState>();
        mAgentStore.Register<AgentLocationState>();
        mAgentStore.Register<AgentAge>();

        mAgentStore.Resize(128);
    }

    void SimDriver::Update(float deltaTime, float totalTime)
    {
        auto uids = mAgentStore.Data<AgentUID>();

    }

    void SimDriver::Destroy()
    {
    }
}