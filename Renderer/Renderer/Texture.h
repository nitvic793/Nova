#pragma once

#include <Lib/Handle.h>

namespace nv::graphics
{
    class GPUResource;

    namespace tex
    {
        enum Type
        {
            NONE = 0,
            TEXTURE_1D,
            TEXTURE_2D,
            TEXTURE_3D,
            TEXTURE_CUBE
        };

        enum Usage
        {
            USAGE_NONE = 0,
            USAGE_SHADER,
            USAGE_UNORDERED,
            USAGE_RENDER_TARGET,
            USAGE_DEPTH_STENCIL
        };
    }

    struct TextureDesc
    {
        tex::Type           mType = tex::TEXTURE_2D;
        tex::Usage          mUsage = tex::USAGE_SHADER;
        uint32_t            mWidth = 0;
        uint32_t            mHeight = 0;
        Handle<GPUResource> mBuffer = Handle<GPUResource>();
    };

    class Texture
    {
    public:
        virtual ~Texture() {}

    private:
        TextureDesc mDesc;
    };


}