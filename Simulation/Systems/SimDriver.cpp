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

    void RunStoreTests()
    {
        mAgentData.Init();

        auto& sats = mAgentData.Get<AgentSatisfaction>();
        auto ages = mAgentData.GetSpan<AgentAge>();

        auto spans = mAgentData.GetSpans<AgentState, AgentLocationState>();
        auto states = std::get<std::span<AgentState>>(spans);

        int i = 0;
        for (auto& s : sats)
        {
            s += i / 10.f;
            i = (i + 1) % 10;
        }

        i = 0;
        for (auto& age : ages)
        {
            age.mValue = (float)i++;
        }

        for (auto& state : states)
        {
            state = AgentState::ASTATE_MOVING_IN;
        }

        auto inst = mAgentData.GetInstance(2);
        auto instRef = mAgentData.GetInstanceRef(2);

        instRef.Get<AgentAge>() = (AgentAge)50.f;
        instRef.Get<Position>() = (Position)math::float3(1, 1, 1);

        mAgentData.ForEach<AgentID, AgentState>(
            [](AgentID& id, AgentState& state)
            {
                if(id.mValue == INVALID_AGENT_ID)
                    state = AgentState::ASTATE_SPAWNED;
            });
    }

    void SimDriver::Init()
    { 
        RunStoreTests();
        
        jobs::InitJobSystem(4);
        mAgentManager = std::make_unique<AgentManager>();
        mAgentManager->Init();
        mAgentManager->Spawn();
    }

    void SimDriver::Update(float deltaTime, float totalTime)
    {

    }

    void SimDriver::Destroy()
    {
    }

}