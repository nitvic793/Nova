#pragma once

#include <stdint.h>
#include <Lib/StringHash.h>
#include <Store/Store.h>
#include <Math/Math.h>

#define PROP(name) static constexpr uint32_t Hash = ID(#name);

namespace nv::sim::agent
{
    template<typename T>
    struct Property
    {
        T mValue = {};

        constexpr operator T() { return mValue; }
        constexpr operator T& () { return mValue; }
        //constexpr operator T&&() { return mValue; }
        constexpr operator T() const { return mValue; }
        constexpr operator const T&() const { return mValue; }

        constexpr T& operator=(T& val) { mValue = val; }
        constexpr T& operator=(T&& val) { mValue = std::move(val); }
        constexpr T& operator=(const T& val)  { mValue = val; }

        constexpr Property(const T& val) : mValue(val) {}
        constexpr Property(T&& val) : mValue(val) {}
        constexpr Property() : mValue(){}
    };

    using FloatProperty     = Property<float>;
    using Float3Property    = Property<math::float3>;
    using UIntProperty      = Property<uint32_t>;
    using UInt64Property    = Property<uint64_t>;

    struct AgentAge             : public FloatProperty {};
    struct AgentSatisfaction    : public FloatProperty {};
    struct AgentID              : public UInt64Property{};
    struct Position             : public Float3Property{};
    struct Wealth               : public UIntProperty  {};

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

    struct AgentUID
    {
        AgentID mUID;
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
}