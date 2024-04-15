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

    void SetEnableDebugDraw(bool enable);
    void SetEnableDebugUI(bool enable);
    bool IsDebugUIEnabled();
    bool IsDebugDrawEnabled();

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