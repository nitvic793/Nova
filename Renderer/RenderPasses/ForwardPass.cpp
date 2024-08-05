#include "pch.h"
#include "ForwardPass.h"

#include <AssetManager.h>
#include <Types/ShaderAsset.h>
#include <Renderer/PipelineState.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Renderer.h>
#include <Renderer/Mesh.h>
#include <Renderer/Device.h>

#include <Debug/Profiler.h>
#if NV_PLATFORM_WINDOWS
#include <DX12/ContextDX12.h>
#endif

namespace nv::graphics
{
    void ForwardPass::Init()
    {
        auto ps = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultPS.hlsl") };
        auto vs = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultVS.hlsl") };
        auto vsAnim = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultAnimationVS.hlsl") };

        PipelineStateDesc psoDesc = {};
        psoDesc.mPipelineType = PIPELINE_RASTER;
        psoDesc.mPS = gResourceManager->CreateShader({ ps, shader::PIXEL });
        psoDesc.mVS = gResourceManager->CreateShader({ vs, shader::VERTEX });
        psoDesc.mNumRenderTargets = 1;
        psoDesc.mRenderTargetFormats[0] = gRenderer->GetDefaultRenderTargetFormat();
        psoDesc.mDepthFormat = gRenderer->GetDepthSurfaceFormat();
        mForwardPSO = gResourceManager->CreatePipelineState(psoDesc);

        psoDesc.mVS = gResourceManager->CreateShader({ vsAnim, shader::VERTEX });
        psoDesc.mbUseAnimLayout = true;
        mForwardAnimPSO = gResourceManager->CreatePipelineState(psoDesc);
    }

    void ForwardPass::Execute(const RenderPassData& renderPassData)
    {
        uint32_t bonesCbIdx = 0;
        auto ctx = gRenderer->GetContext();
        NV_GPU_EVENT("ForwardPass", ((ContextDX12*)ctx)->GetCommandList());

        // TODO:
        // Forward pass pipeline should be material type agnostic, DefaultPS.hlsl should check for type of material and then draw
        const auto bindAndDrawObject = [&](ConstantBufferView objCb, ConstantBufferView matCb, Mesh* mesh, ConstantBufferView* bonesCb)
        {
            ctx->BindConstantBuffer(0, (uint32_t)objCb.mHeapIndex);
            if (bonesCb)
            {
                ctx->BindConstantBuffer(4, (uint32_t)bonesCb->mHeapIndex);
                bonesCbIdx++;
            }

            ctx->SetMesh(mesh);
            for (auto entry : mesh->GetDesc().mMeshEntries)
            {
                ctx->DrawIndexedInstanced(entry.mNumIndices, 1, entry.mBaseIndex, entry.mBaseVertex, 0);
            }
        };

        SetContextDefault(ctx);
        ctx->SetPipeline(mForwardPSO);
        auto lightAccumTex = gResourceManager->GetTextureHandle(ID("RTPass/LightAccumBufferSRV"));
        ctx->BindTexture(1, lightAccumTex);
        ctx->Bind(3, BIND_BUFFER, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);

        auto objectCbs = renderPassData.mRenderDataArray.GetObjectDescriptors();
        auto materialCbs = renderPassData.mRenderDataArray.GetMaterialDescriptors();
        auto boneCbs = renderPassData.mRenderDataArray.GetBoneDescriptors();
        for (size_t i = 0; i < objectCbs.Size(); ++i)
        {
            const auto& objectCb = objectCbs[i];
            const auto& matCb = materialCbs[i];
            const auto mesh = renderPassData.mRenderData[i].mpMesh;

            RenderDescriptors::CBV* boneCb = nullptr;
            if (mesh)
            {      
                if (mesh->HasBones())
                {
                    boneCb = &boneCbs[bonesCbIdx];
                    ctx->SetPipeline(mForwardAnimPSO);
                }
                else
                {
                    ctx->SetPipeline(mForwardPSO);
                }

                bindAndDrawObject(objectCb, matCb, mesh, boneCb); // Increments bonesCbIdx
            }
        }
    }

    void ForwardPass::Destroy()
    {
    }
}

