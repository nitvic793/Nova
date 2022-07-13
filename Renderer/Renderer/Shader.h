#pragma once

namespace nv::graphics
{
    namespace shader
    {
        enum Type
        {
            VERTEX,
            PIXEL,
            COMPUTE
        };
    }

    struct ShaderDesc
    {
        shader::Type mType;
    };

    class Shader
    {
    public:


    protected:
    };
}