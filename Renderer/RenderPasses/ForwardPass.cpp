#include "pch.h"
#include "ForwardPass.h"

#include <AssetManager.h>
#include <Types/ShaderAsset.h>
#include <Renderer/PipelineState.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Renderer.h>
#include <Renderer/Mesh.h>
#include <Renderer/Device.h>

namespace nv::graphics
{
    void ForwardPass::Init()
    {
        auto ps = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultPS.hlsl") };
        auto vs = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultVS.hlsl") };

        PipelineStateDesc psoDesc = {};
        psoDesc.mPipelineType = PIPELINE_RASTER;
        psoDesc.mPS = gResourceManager->CreateShader({ ps, shader::PIXEL });
        psoDesc.mVS = gResourceManager->CreateShader({ vs, shader::VERTEX });
        psoDesc.mNumRenderTargets = 1;
        psoDesc.mRenderTargetFormats[0] = gRenderer->GetDefaultRenderTargetFormat();
        psoDesc.mDepthFormat = gRenderer->GetDepthSurfaceFormat();
        mForwardPSO = gResourceManager->CreatePipelineState(psoDesc);
    }

    void ForwardPass::Execute(const RenderPassData& renderPassData)
    {
        auto ctx = gRenderer->GetContext();
        const auto bindAndDrawObject = [&](ConstantBufferView objCb, ConstantBufferView matCb, Mesh* mesh)
        {
            ctx->BindConstantBuffer(0, (uint32_t)objCb.mHeapIndex);
            ctx->BindConstantBuffer(1, (uint32_t)matCb.mHeapIndex);
            ctx->SetMesh(mesh);
            for (auto entry : mesh->GetDesc().mMeshEntries)
            {
                ctx->DrawIndexedInstanced(entry.mNumIndices, 1, entry.mBaseIndex, entry.mBaseVertex, 0);
            }
        };

        const auto topology = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto renderTarget = gRenderer->GetDefaultRenderTarget();
        const auto depthTarget = gRenderer->GetDefaultDepthTarget();
        const auto gpuHeap = gRenderer->GetGPUDescriptorHeap();
        Handle<Texture> targets[] = { renderTarget };
        Handle<DescriptorHeap> heaps[] = { gpuHeap };

        auto rect = gRenderer->GetDefaultScissorRect();
        auto viewport = gRenderer->GetDefaultViewport();

        ctx->SetScissorRect(1, &rect);
        ctx->SetViewports(1, &viewport);
        ctx->SetPrimitiveTopology(topology);
        ctx->SetDescriptorHeap({ heaps, _countof(heaps) });

        ctx->SetRenderTarget({ targets, _countof(targets) }, depthTarget);
        ctx->SetPipeline(mForwardPSO);
        ctx->Bind(4, BIND_BUFFER, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);

        auto objectCbs = renderPassData.mRenderDataArray.GetObjectDescriptors();
        auto materialCbs = renderPassData.mRenderDataArray.GetMaterialDescriptors();
        for (size_t i = 0; i < objectCbs.Size(); ++i)
        {
            const auto& objectCb = objectCbs[i];
            const auto& matCb = materialCbs[i];
            const auto mesh = renderPassData.mRenderData[i].mpMesh;
            if (mesh)
                bindAndDrawObject(objectCb, matCb, mesh);
        }
    }

    void ForwardPass::Destroy()
    {
    }
}

