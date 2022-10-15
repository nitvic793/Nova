#pragma once

#include <Renderer/RenderPass.h>

namespace nv::graphics
{
    class PipelineState;

    class Skybox : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(Skybox)
        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
        Handle<PipelineState> mSkyboxPSO;
    };
}