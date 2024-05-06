#include "pch.h"

#include <tuple>
#include <utility> 

#include "SimDriver.h"
#include <Agent/Agent.h>
#include <Engine/JobSystem.h>


namespace nv
{
    using namespace sim::agent;
    using namespace sim;

    ArchetypeStore<AgentArchetype> mAgentData;

    void SimDriver::Init()
    { 
        mAgentData.Init();

        auto& sats = mAgentData.Get<AgentSatisfaction>();
        auto& ages = mAgentData.Get<AgentAge>();

        int i = 0;
        for (auto& s : sats)
        {
            s += i/10.f;
            i = (i + 1) % 10;
        }
        
        jobs::InitJobSystem(4);
        mAgentManager = std::make_unique<AgentManager>();
        mAgentManager->Init();
    }

    void SimDriver::Update(float deltaTime, float totalTime)
    {

    }

    void SimDriver::Destroy()
    {
    }

}