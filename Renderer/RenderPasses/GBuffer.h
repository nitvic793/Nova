#pragma once

#include <Renderer/RenderPass.h>

namespace nv::graphics
{
    class PipelineState;

    class GBuffer : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(GBuffer)
        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
        Handle<PipelineState> mGBufferPSO;
    };
}