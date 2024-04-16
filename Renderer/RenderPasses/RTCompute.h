#pragma once

#include <Renderer/RenderPass.h>

namespace nv::graphics
{
    class PipelineState;

    class RTCompute : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(RTCompute);

        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
        Handle<PipelineState> mRTComputePSO;
        Handle<PipelineState> mAccumulatePSO;
        Handle<PipelineState> mBlurPSO;
    };
}