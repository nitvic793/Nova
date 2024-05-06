#pragma once

#include <stdint.h>
#include <Lib/StringHash.h>

#define PROP(name) static constexpr uint32_t Hash = ID(#name);

namespace nv::sim::agent
{
    enum class AgentState : uint8_t
    {
        AGENT_NONE,
        AGENT_IDLE,
        AGENT_MOVING_IN,
        AGENT_WORKING,
        AGENT_STUDYING,
        AGENT_SHOPPING,
        AGENT_FREE_TIME,
        AGENT_COMMUTING,
        AGENT_DEAD
    };

    enum class AgentLocationState : uint8_t
    {
        AGENT_NONE,
        AGENT_HOME,
        AGENT_WORK,
        AGENT_OUT_SHOPPING,
        AGENT_OUT_FREETIME,
        AGENT_HOMELESS
    };

    struct Float3
    {
        float X;
        float Y;
        float Z;
    };

    using AgentID = uint32_t;

    struct AgentUID
    {
        AgentID mUID;
    };

    struct Position
    {
        Float3 mValue;
    };

    using AgentAge = float;

}