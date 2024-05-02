#pragma once

#include <Renderer/CommonDefines.h>
#include <Renderer/Format.h>

namespace nv::graphics
{
    class GPUResource;

    struct TransitionBarrier
    {
        buffer::State       mFrom       = buffer::STATE_COMMON;
        buffer::State       mTo         = buffer::STATE_COMMON;
        Handle<GPUResource> mResource   = Null<GPUResource>();
    };

    struct UAVBarrier
    {
        Handle<GPUResource> mResource = Null<GPUResource>();
    };

    struct ResourceClearValue
    {
        format::SurfaceFormat   mFormat     = format::UNKNOWN;
        float                   mColor[4]   = { 0.f, 0.f, 0.f, 0.f };
        uint8_t                 mStencil    = 0;
        bool                    mIsDepth    = false;
    };

    struct GPUResourceDesc
    {
        union 
        {
            uint32_t            mWidth          = 0;
            uint32_t            mSize;
        };
        uint32_t                mHeight         = 0;
        format::SurfaceFormat   mFormat         = format::UNKNOWN;
        buffer::Type            mType           = buffer::TYPE_BUFFER;
        buffer::Flags           mFlags          = buffer::FLAG_NONE;
        buffer::State           mInitialState   = buffer::STATE_COMMON;
        uint32_t                mArraySize      = 1;
        uint32_t                mMipLevels      = 0;
        uint32_t                mSampleCount    = 1;
        uint32_t                mSampleQuality  = 0;
        ResourceClearValue      mClearValue     = {};
        buffer::BufferMode      mBufferMode     = buffer::BUFFER_MODE_DEFAULT;

        static GPUResourceDesc UploadConstBuffer(uint32_t size)
        {
            return 
            {
                .mSize = size,
                .mType = buffer::TYPE_BUFFER,
                .mInitialState = buffer::STATE_GENERIC_READ,
                .mBufferMode = buffer::BUFFER_MODE_UPLOAD
            };
        }

        static GPUResourceDesc Texture2D(uint32_t width, uint32_t height, buffer::Flags flags = buffer::FLAG_NONE, buffer::State initState = buffer::STATE_COMMON, format::SurfaceFormat format = format::R8G8B8A8_UNORM, uint32_t mipLevels = 1, uint32_t sampleCount = 1)
        {
            return
            {
                .mWidth = width,
                .mHeight = height,
                .mFormat = format,
                .mType = buffer::TYPE_TEXTURE_2D,
                .mFlags = flags,
                .mInitialState = initState,
                .mMipLevels = mipLevels,
                .mSampleCount = sampleCount
            };
        }
    };

    class GPUResource : public ResourceBase
    {
    public:
        GPUResource(const GPUResourceDesc& desc) :
            mDesc(desc),
            mMappedAddress(nullptr),
            mCurrentState(desc.mInitialState)
        {}

        virtual ~GPUResource() {}

        constexpr const GPUResourceDesc&    GetDesc() const { return mDesc; }
        constexpr uint8_t*                  GetMappedMemory() const { return mMappedAddress; };
        constexpr buffer::State             GetResourceState() const { return mCurrentState; }
        constexpr void                      UpdateResourceState(buffer::State state) { mCurrentState = state; }

        virtual void                        MapMemory() = 0;
        virtual void                        UnmapMemory() = 0;
        virtual void                        UploadMapped(uint8_t* bytes, size_t size, size_t offset = 0) = 0;

    protected:
        GPUResourceDesc mDesc;
        uint8_t*        mMappedAddress;
        buffer::State   mCurrentState;
    };
}