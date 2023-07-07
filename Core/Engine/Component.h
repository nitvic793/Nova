#pragma once

#include <string>

namespace nv::ecs
{
    struct IComponent
    {
        template<typename T>
        T* As()
        {
            return static_cast<T*>(this);
        }
    };

    template<typename TComp>
    constexpr std::string_view GetComponentName()
    {
        std::string_view compName = TypeName<TComp>();
        constexpr std::string_view prefix = "struct ";
        compName.remove_prefix(prefix.size());
        return compName;
    }

    template<typename TComp>
    constexpr StringID GetComponentID()
    {
        std::string_view compName = TypeName<TComp>();
        constexpr std::string_view prefix = "struct ";
        compName.remove_prefix(prefix.size());
        return ID(compName);
    }
}