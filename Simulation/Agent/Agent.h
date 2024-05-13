#pragma once

#include <stdint.h>
#include <Lib/StringHash.h>
#include <Store/Store.h>
#include <Math/Math.h>

namespace nv::sim::agent
{
    // Index Key types should inherit from StoreIndex
    struct AgentID              : public UInt64Property, 
                                  public StoreIndex    {};

    struct AgentAge             : public FloatProperty {};
    struct AgentSatisfaction    : public FloatProperty {};
    struct Position             : public Float3Property{};
    struct Wealth               : public UIntProperty  {};

    static constexpr uint64_t INVALID_AGENT_ID = 0;

    enum class AgentState : uint8_t
    {
        ASTATE_NONE,
        ASTATE_SPAWNED,
        ASTATE_IDLE,
        ASTATE_MOVING_IN,
        ASTATE_WORKING,
        ASTATE_STUDYING,
        ASTATE_SHOPPING,
        ASTATE_FREE_TIME,
        ASTATE_COMMUTING,
        ASTATE_SICK,
        ASTATE_DEAD
    };

    enum class AgentLocationState : uint8_t
    {
        ALOCSTATE_NONE,
        ALOCSTATE_TRAVELING,
        ALOCSTATE_HOME,
        ALOCSTATE_WORK,
        ALOCSTATE_OUT_SHOPPING,
        ALOCSTATE_OUT_FREETIME,
        ALOCSTATE_HOMELESS
    };

    using AgentArchetype = Archetype<
        AgentID,
        AgentState,
        AgentLocationState,
        AgentAge,
        AgentSatisfaction,
        Position,
        Wealth
    >;

    using AgentStore = ArchetypeStore<AgentArchetype>;

    // TODO: Residence, Office, Industrial and Commercial Archetypes
}