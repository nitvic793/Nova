#pragma once

#include <Store/Store.h>
#include <Agent/Agent.h>

namespace nv::sim
{
    class AgentManager
    {
        static constexpr uint32_t INIT_AGENT_COUNT = 128;
    public:
        void Init();

        template<typename T>
        std::span<T> Data() const { return mAgentStore.Data<T>(); }

    private:
        sim::Store mAgentStore;
    };
}