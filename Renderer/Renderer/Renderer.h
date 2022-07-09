#pragma once

#include <Lib/Handle.h>

namespace nv::graphics
{
    class Shader;
    class GPUResource;

    class IRenderer
    {
    public:
        virtual void Draw() {}
    private:

    };
}