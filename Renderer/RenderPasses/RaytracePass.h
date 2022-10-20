#pragma once

#include <Renderer/RenderPass.h>

namespace nv::graphics
{
    class PipelineState;
    struct RTObjects;

    class RaytracePass : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(RaytracePass)
        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
        void CreateRaytracingStructures(const RenderPassData& renderPassData);
        void CreateOutputBuffer();
        void CreatePipeline();
        void CreateShaderBindingTable(const RenderPassData& renderPassData);

    private:
        RTObjects* mRtObjects;
    };
}