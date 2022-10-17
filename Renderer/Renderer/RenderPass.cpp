#include "pch.h"
#include "RenderPass.h"

#include <Renderer/Renderer.h>
#include <Renderer/Device.h>

namespace nv::graphics
{
    void RenderPass::SetContextDefault(Context* context)
    {
        const auto topology = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto renderTarget = gRenderer->GetDefaultRenderTarget();
        const auto depthTarget = gRenderer->GetDefaultDepthTarget();
        const auto gpuHeap = gRenderer->GetGPUDescriptorHeap();
        Handle<Texture> targets[] = { renderTarget };
        Handle<DescriptorHeap> heaps[] = { gpuHeap };

        auto rect = gRenderer->GetDefaultScissorRect();
        auto viewport = gRenderer->GetDefaultViewport();

        context->SetScissorRect(1, &rect);
        context->SetViewports(1, &viewport);
        context->SetPrimitiveTopology(topology);
        context->SetDescriptorHeap({ heaps, _countof(heaps) });
        context->SetRenderTarget({ targets, _countof(targets) }, depthTarget);
    }
}

