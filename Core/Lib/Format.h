#ifndef NV_FORMAT
#define NV_FORMAT

#pragma once

#include <fmt/core.h>

namespace nv
{
    template<typename ...Params>
    constexpr std::string Format(const char* format, Params&& ...args)
    {
        return fmt::format(format, std::forward<Params>(args)...);
    }
}

#endif
