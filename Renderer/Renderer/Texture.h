#pragma once

#include <Lib/Handle.h>
#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    class GPUResource;

    struct TextureDesc
    {
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