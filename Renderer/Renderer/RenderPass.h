#pragma once

#include <Renderer/Texture.h>
#include <Renderer/CommonDefines.h>
#include <Renderer/DescriptorHeap.h>
#include <Renderer/Context.h>
#include <Renderer/RenderDataArray.h>
#include <Lib/Vector.h>
#include <Interop/ShaderInteropTypes.h>

namespace nv::graphics
{
    enum RenderPassType
    {
        RENDERPASS_UNDEFINED = 0,
        RENDERPASS_OPAQUE,
        RENDERPASS_EFFECTS,
        RENDERPASS_TRANSPARENT,
        RENDERPASS_POSTPROCESS,
        RENDERPASS_UI,
        RENDERPASS_DEBUG
    };

    struct RenderPassData
    {
        FrameData           mFrameData;
        ConstantBufferView  mFrameDataCBV;
        RenderData&         mRenderData;
        RenderDataArray&    mRenderDataArray;
    };

    class RenderPass
    {
    public:
        virtual void Init() {}
        virtual void Execute(const RenderPassData& renderPassData) = 0;
        virtual void Destroy() {}

        virtual RenderPassType GetRenderPassType() const { return RENDERPASS_OPAQUE; }
    };
}