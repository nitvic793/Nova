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

    template <typename... Types> 
    class Data
    {
        static constexpr uint32_t INIT_SIZE = 32;

    public:
        void Init()
        {
            std::apply(
                [](auto&&... args) 
                {
                    ((args.resize(INIT_SIZE)), ...); 
                }, 
                mItems);
        }

        template<typename T>
        std::vector<T>& Get()
        {
            std::vector<T>& items = std::get<std::vector<T>>(mItems);
            return items;
        }

    private:
        std::tuple<std::vector<Types>...> mItems;
    };


    template<typename... Types>
    struct Archetype
    {
        template <template <typename...> typename T>
        using Apply = T<Types...>;

        using Store = Apply<Data>;
    };

    using AgentArchetype = Archetype<AgentID, AgentState, AgentLocationState, AgentAge, AgentSatisfaction>;

    void SimDriver::Init()
    {
        AgentArchetype::Store mAgentData;
        mAgentData.Init();

        auto& sats = mAgentData.Get<AgentSatisfaction>();
        auto& ages = mAgentData.Get<AgentAge>();

        int i = 0;
        for (auto& s : sats)
        {
            s.mValue += i/10.f;
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