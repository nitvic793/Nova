#pragma once

#include <cstdint>

namespace nv
{
    template<typename T>
    struct Handle
    {
    public:
        union
        {
            struct
            {
                uint32_t mIndex;
                uint32_t mGeneration;
            };
            uint64_t mHandle;
        };

        constexpr Handle() : mHandle(0) {}
        constexpr bool IsNull() { return mGeneration == 0; }

    private:
        constexpr Handle(uint32_t index, uint32_t gen) :
            mIndex(index),
            mGeneration(gen) {}

        template<typename U, typename Gen, uint32_t>
        friend class Pool;
    };
}
