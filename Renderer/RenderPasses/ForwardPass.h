#pragma once

#include <Renderer/RenderPass.h>

namespace nv::graphics
{
    class PipelineState;

    class ForwardPass : public RenderPass
    {
    public:
        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
        Handle<PipelineState> mForwardPSO;
    };
}