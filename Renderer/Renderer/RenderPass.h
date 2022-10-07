#pragma once

#include <Renderer/Texture.h>
#include <Renderer/CommonDefines.h>
#include <Renderer/DescriptorHeap.h>
#include <Renderer/Context.h>
#include <Lib/Vector.h>
#include <Interop/ShaderInteropTypes.h>

namespace nv::graphics
{
    enum RenderPassType
    {
        RENDERPASS_OPAQUE,
        RENDERPASS_TRANSPARENT,
        RENDERPASS_POSTPROCESS
    };

    struct RenderPass
    {
        nv::Vector<Handle<Texture>> mRenderTargets;
        Handle<DescriptorHeap>      mDescriptorHeaps;
        Handle<Texture>             mDepthTarget        = Null<Texture>();
        PrimitiveTopology           mPrimitiveTopology  = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        Viewport                    mViewport;
        Rect                        mScissorRect;
        FrameData                   mFrameData;
        RenderPassType              mRenderPassType     = RENDERPASS_OPAQUE;

        void Begin(Context* context) {};
        virtual void RunPass() {};

    };
}