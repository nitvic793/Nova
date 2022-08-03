#pragma once

namespace nv::graphics
{
    namespace shader
    {
        enum Type : uint8_t
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
        Shader(const ShaderDesc& desc) :
            mDesc(desc) {}
        virtual ~Shader() {}

    protected:
        ShaderDesc mDesc;
    };
}