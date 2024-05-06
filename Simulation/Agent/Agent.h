#pragma once

#include <stdint.h>
#include <Lib/StringHash.h>
#include <Store/Store.h>

#define PROP(name) static constexpr uint32_t Hash = ID(#name);

namespace nv::sim::agent
{
    template<typename T>
    struct Property
    {
        T mValue = {};

        operator T() { return mValue; }
        operator T& () { return mValue; }
        operator T&&() { return mValue; }
        operator T() const { return mValue; }
        operator const T&() const { return mValue; }

        Property(const T& val) : mValue(val) {}
        Property(T&& val) : mValue(val) {}
        Property() : mValue(){}
    };

    using FloatProperty = Property<float>;
    using UIntProperty = Property<uint32_t>;

    struct AgentAge : public FloatProperty {};
    struct AgentSatisfaction : public FloatProperty {};
    struct AgentID : public UIntProperty {};

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

    struct AgentUID
    {
        AgentID mUID;
    };

    struct Position
    {
        Float3 mValue;
    };

    using AgentArchetype = Archetype<
        AgentID,
        AgentState,
        AgentLocationState,
        AgentAge,
        AgentSatisfaction
    >;
}