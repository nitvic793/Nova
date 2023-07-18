#pragma once

#include <Renderer/RenderPass.h>

namespace DirectX
{
    class GraphicsMemory;
}

namespace nv::graphics
{
    class DebugDrawPass : public RenderPass
    {
    public:
        NV_RENDER_PASS_NAME(DebugDrawPass)
        void Init() override;
        void Execute(const RenderPassData& renderPassData) override;
        void Destroy() override;

    private:
    };


#if NV_RENDERER_DX12
    GraphicsMemory* GetGraphicsMemory();
#endif
}