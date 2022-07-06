#ifndef NV_LOG
#define NV_LOG

#pragma once

#include <fmt/core.h>
#include <fmt/printf.h>
#include <fmt/color.h>
#include <fmt/os.h>

namespace nv::log
{
    template<typename ...Params>
    constexpr void Log(FILE* f, const char* format, Params&& ...args)
    {
        fmt::print(f, fmt::format("{}\n", format), std::forward<Params>(args)...);
    }

    template<typename ...Params>
    constexpr void Error(FILE* f, const char* format, Params&& ...args)
    {
        fmt::print(f, fg(fmt::color::crimson) | fmt::emphasis::bold, fmt::format("[Error] {}\n", format), std::forward<Params>(args)...);
    }

    template<typename ...Params>
    constexpr void Info(FILE* f, const char* format, Params&& ...args)
    {
        fmt::print(f, fg(fmt::color::light_blue), fmt::format("[Info] {}\n", format), std::forward<Params>(args)...);
    }

    template<typename ...Params>
    constexpr void Warn(FILE* f, const char* format, Params&& ...args)
    {
        fmt::print(f, fg(fmt::color::yellow), fmt::format("[Warn] {}\n", format), std::forward<Params>(args)...);
    }

    template<typename ...Params>
    constexpr void Log(const char* format, Params&& ...args)
    {
        Log(stdout, format, std::forward<Params>(args)...);
    }

    template<typename ...Params>
    constexpr void Error(const char* format, Params&& ...args)
    {
        Error(stderr, format, std::forward<Params>(args)...);
    }

    template<typename ...Params>
    constexpr void Info(const char* format, Params&& ...args)
    {
        Info(stdout, format, std::forward<Params>(args)...);
    }

    template<typename ...Params>
    constexpr void Warn(const char* format, Params&& ...args)
    {
        Warn(stdout, format, std::forward<Params>(args)...);
    }
}


#endif