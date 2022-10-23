#pragma once

#include <Lib/Handle.h>
#include <Renderer/Format.h>
#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    class GPUResource;



    struct TextureDesc
    {
        tex::Usage              mUsage = tex::USAGE_SHADER;
        format::SurfaceFormat   mFormat = format::FORMAT_UNKNOWN;
        Handle<GPUResource>     mBuffer = Handle<GPUResource>();
        tex::Type               mType = tex::TEXTURE_2D;
        bool                    mUseRayTracingHeap = false;
        tex::Buffer             mBufferData = {};
    };

    class Texture
    {
    public:
        Texture(const TextureDesc& desc) :
            mDesc(desc) {}
        virtual ~Texture() {}

        constexpr const TextureDesc& GetDesc() const { return mDesc; }
        constexpr Handle<GPUResource> GetBuffer() const { return mDesc.mBuffer; }
        constexpr tex::Type GetType() const { return mDesc.mType; }
        constexpr tex::Usage GetUsage() const { return mDesc.mUsage; }

        virtual uint32_t GetHeapIndex() const = 0;
        virtual uint32_t GetHeapOffset() const = 0;

    protected:
        TextureDesc mDesc;
    };


}