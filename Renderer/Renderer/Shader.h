#pragma once

#include <Asset.h>

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
        shader::Type    mType;
        asset::AssetID  mShader;
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