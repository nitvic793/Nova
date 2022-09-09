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

        enum ShaderModel : uint8_t
        {
            SM_6_5,
            SM_6_6,
            SM_6_7
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