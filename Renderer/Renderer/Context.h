#pragma once

#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    struct ContextDesc
    {
        ContextType mType;
    };

    class CommandBuffer
    {

    };

    class Context
    {
    public:
        virtual ~Context() {}

    protected:
    };
}