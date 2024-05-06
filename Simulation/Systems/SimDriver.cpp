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
        auto ages = mAgentData.GetSpan<AgentAge>();

        int i = 0;
        for (auto& s : sats)
        {
            s += i/10.f;
            i = (i + 1) % 10;
        }

        i = 0;
        for (auto& age : ages)
        {
            age.mValue =  (float) i++;
        }

        auto inst = mAgentData.GetInstance(2);
        auto instRef = mAgentData.GetInstanceRef(2);
        std::get<AgentAge&>(instRef) = (AgentAge)50.f;
        std::get<Position&>(instRef) = (Position)math::float3(1, 1, 1);
        
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