#pragma once

#include <cstdint>  
#include <atomic>

namespace nv::graphics
{
    enum class ResourceLoadStateEnum : uint8_t
    {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };

    using ResourceLoadState = std::atomic<ResourceLoadStateEnum>;
}