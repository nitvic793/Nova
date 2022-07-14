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
        Handle<GPUResource> mResource   = Handle<GPUResource>();
    };

    struct GPUResourceDesc
    {
        uint32_t                mWidth          = 0;
        uint32_t                mHeight         = 0;
        uint32_t                mArraySize      = 1;
        uint32_t                mMipLevels      = 0;
        uint32_t                mSampleCount    = 1;
        uint32_t                mSampleQuality  = 0;
        buffer::Type            mType           = buffer::TYPE_BUFFER;
        buffer::Flags           mFlags          = buffer::FLAG_NONE;
        buffer::State           mInitialState   = buffer::STATE_COMMON;
        format::SurfaceFormat   mFormat         = format::UNKNOWN;
    };

    class GPUResource
    {
    public:
        virtual ~GPUResource() {}
    };
}