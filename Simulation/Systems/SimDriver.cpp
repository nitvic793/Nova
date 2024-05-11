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

    void Process(AgentID& id, AgentState& state)
    {
        if (id.mValue == INVALID_AGENT_ID)
            state = AgentState::ASTATE_SPAWNED;
    }

    class Processor : public IProcessor<AgentStore, Processor>
    {
    public:
        constexpr void Process(AgentID& id, AgentState& state)
        {
            if (id.mValue == INVALID_AGENT_ID)
                state = AgentState::ASTATE_SPAWNED;
        }
    };

    class BatchProcessor : public IBatchProcessor<AgentStore, BatchProcessor>
    {
    public:
        constexpr void Process(std::span<AgentID> ids, std::span<AgentState> states)
        {
            for (size_t i = 0; i < ids.size(); ++i)
            {
                auto& id = ids[i];
                auto& state = states[i];

                if (id.mValue == INVALID_AGENT_ID)
                    state = AgentState::ASTATE_SPAWNED;
            }
        }
    };

    void RunStoreTests()
    {
        mAgentData.Init();
        mAgentData.Resize(1'000'000);

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

        Processor processor;
        mAgentData.ForEach(&Process);
        mAgentData.ForEach(&Processor::Process, processor);
        processor.Invoke(mAgentData);

        mAgentData.RegisterProcessor<Processor>();
        mAgentData.Tick();

        BatchProcessor batch;
        batch.Invoke(mAgentData);
    }

    void SimDriver::Init()
    { 
        sgDataStoreFactory.Register<AgentStore>();
        mTimer = sim::SimTimer{ .mDay = 1,  .mMonth = 1, .mYear = 2020, .mSimSpeed = SimSpeed::SIMSPEED_NORMAL };
        jobs::InitJobSystem(4);
        RunStoreTests();
        mAgentManager = std::make_unique<AgentManager>();
        mAgentManager->Init();
        mAgentManager->Spawn();
    }

    void SimDriver::Update(float deltaTime, float totalTime)
    {
        mTimer.Tick(deltaTime);
    }

    void SimDriver::Destroy()
    {
    }

}