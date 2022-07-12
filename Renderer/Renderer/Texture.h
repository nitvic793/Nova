#pragma once

#include <Lib/Handle.h>

namespace nv::graphics
{
    class GPUResource;

    namespace tex
    {
        enum Type
        {
            kTextureTypeNone = 0,
            kTexture1D,
            kTexture2D,
            kTexture3D,
            kTextureCube
        };

        enum Usage
        {
            kTexUsageNone = 0,
            kTexUsageShader,
            kTexUsageUnordered,
            kTexUsageRenderTarget,
            kTexUsageDepthStencil
        };
    }

    struct TextureDesc
    {
        tex::Type           mType = tex::kTexture2D;
        tex::Usage          mUsage = tex::kTexUsageShader;
        uint32_t            mWidth = 0;
        uint32_t            mHeight = 0;
        Handle<GPUResource> mBuffer = Handle<GPUResource>();
    };

    class Texture
    {
    public:
        virtual void Create(const TextureDesc& desc) = 0;

        virtual ~Texture() {}
    private:
        TextureDesc mDesc;
    };


}