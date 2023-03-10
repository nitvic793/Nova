#pragma once

#include <Renderer/RenderPass.h>

namespace nv::graphics
{
#if NV_ENABLE_DEBUG_UI
    class DebugUIPass : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(DebugUIPass)
        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
    };
#else
    // Empty implementation
    class DebugUIPass : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(DebugUIPass);
        void Init() override {}
        void Execute(const RenderPassData& renderPassData) override {}
        void Destroy() override {}

    private:
    };
#endif
}