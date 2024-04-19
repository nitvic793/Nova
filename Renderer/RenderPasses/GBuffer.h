#pragma once

#include <Renderer/RenderPass.h>

namespace nv::graphics
{
    class PipelineState;
    class GPUResource;

    class GBuffer : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(GBuffer)
        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
        static constexpr uint32_t GBUFFER_COUNT = 4;
        Handle<PipelineState> mGBufferPSO;
        Handle<GPUResource> mGBufferA;
        Handle<GPUResource> mGBufferB;
        Handle<GPUResource> mGBufferC;
        Handle<GPUResource> mGBufferD;
        Handle<GPUResource> mDepthBuffer;

        Handle<Texture> mGBufferRenderTargets[GBUFFER_COUNT];
        Handle<Texture> mDepthTarget;
    };
}