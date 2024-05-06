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
        ASTATE_NONE,
        ASTATE_IDLE,
        ASTATE_MOVING_IN,
        ASTATE_WORKING,
        ASTATE_STUDYING,
        ASTATE_SHOPPING,
        ASTATE_FREE_TIME,
        ASTATE_COMMUTING,
        ASTATE_DEAD
    };

    enum class AgentLocationState : uint8_t
    {
        ALOCSTATE_NONE,
        ALOCSTATE_HOME,
        ALOCSTATE_WORK,
        ALOCSTATE_OUT_SHOPPING,
        ALOCSTATE_OUT_FREETIME,
        ALOCSTATE_HOMELESS
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