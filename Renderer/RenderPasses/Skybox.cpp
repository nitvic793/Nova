#include "pch.h"
#include "Skybox.h"

#include <Renderer/PipelineState.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Shader.h>
#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Asset.h>

namespace nv::graphics
{
    using namespace components;
    using namespace asset;
    using namespace ecs;

    void Skybox::Init()
    {
        auto ps = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/SkyboxPS.hlsl") };
        auto vs = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/SkyboxVS.hlsl") };

        PipelineStateDesc psoDesc = {};
        psoDesc.mPipelineType = PIPELINE_RASTER;
        psoDesc.mPS = gResourceManager->CreateShader({ ps, shader::PIXEL });
        psoDesc.mVS = gResourceManager->CreateShader({ vs, shader::VERTEX });
        psoDesc.mNumRenderTargets = 1;
        psoDesc.mRenderTargetFormats[0] = gRenderer->GetDefaultRenderTargetFormat();
        psoDesc.mDepthFormat = gRenderer->GetDepthSurfaceFormat();

        psoDesc.mDepthStencilState.mDepthEnable = true;
        psoDesc.mDepthStencilState.mDepthWriteMask = DEPTH_WRITE_MASK_ALL;
        psoDesc.mDepthStencilState.mDepthFunc = COMPARISON_FUNC_LESS_EQUAL;

        psoDesc.mRasterizerState.mDepthClipEnable = true;
        psoDesc.mRasterizerState.mCullMode = CULL_MODE_FRONT;
        psoDesc.mRasterizerState.mFillMode = FILL_MODE_SOLID;
        mSkyboxPSO = gResourceManager->CreatePipelineState(psoDesc);
    }

    void Skybox::Execute(const RenderPassData& renderPassData)
    {
        auto skyboxes = gComponentManager.GetComponents<SkyboxComponent>();
        if (skyboxes.Size() > 0)
        {
            auto skybox = skyboxes[0].mSkybox;
            auto cubeMesh = gResourceManager->GetMeshHandle(ID("Mesh/cube.obj"));
            auto mesh = gResourceManager->GetMesh(cubeMesh);
            auto ctx = gRenderer->GetContext();

            SetContextDefault(ctx);
            ctx->SetPipeline(mSkyboxPSO);

            ctx->BindTexture(1, skybox);
            ctx->BindConstantBuffer(3, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);

            ctx->SetMesh(mesh);
            for (auto entry : mesh->GetDesc().mMeshEntries)
            {
                ctx->DrawIndexedInstanced(entry.mNumIndices, 1, entry.mBaseIndex, entry.mBaseVertex, 0);
            }
        }
    }

    void Skybox::Destroy()
    {
    }
}

