#include "pch.h"
#include "GBuffer.h"

#include <Renderer/CommonDefines.h>
#include <Renderer/PipelineState.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Shader.h>
#include <Renderer/Window.h>
#include <Renderer/GPUResource.h>
#include <Renderer/Mesh.h>

#include <Debug/Profiler.h>
#if NV_PLATFORM_WINDOWS
#include <DX12/ContextDX12.h>
#endif

namespace nv::graphics
{
    using namespace buffer;

    static Handle<PipelineState> CreateGBufferPSO(nv::Span<format::SurfaceFormat> formats, format::SurfaceFormat depthFormat)
    {
        auto ps = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/GBufferPS.hlsl") };
        auto vs = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/DefaultVS.hlsl") };

        PipelineStateDesc psoDesc = {};
        psoDesc.mPipelineType = PIPELINE_RASTER;
        psoDesc.mPS = gResourceManager->CreateShader({ ps, shader::PIXEL }); 
        psoDesc.mVS = gResourceManager->CreateShader({ vs, shader::VERTEX }); // Will return existing handle if it already exists
        psoDesc.mNumRenderTargets = (uint32_t)formats.Size();
        for (uint32_t i = 0; i < formats.Size(); i++)
        {
			psoDesc.mRenderTargetFormats[i] = formats[i];
		}
        psoDesc.mDepthFormat = depthFormat;
        return gResourceManager->CreatePipelineState(psoDesc);
    }

    void GBuffer::Init()
    {
        uint32_t height = gWindow->GetHeight();
        uint32_t width = gWindow->GetWidth();

        ResourceClearValue matClear = { .mFormat = format::R8G8B8A8_UNORM, .mColor = {0,0,0,0} };
        ResourceClearValue worldPosClear = { .mFormat = format::R16G16B16A16_FLOAT, .mColor = {0,0,0,0} };

        GPUResourceDesc materialBufferDesc = GPUResourceDesc::Texture2D(width, height, FLAG_ALLOW_RENDER_TARGET, STATE_PIXEL_SHADER_RESOURCE);
        materialBufferDesc.mClearValue = matClear;
        GPUResourceDesc velocityBufferDesc = GPUResourceDesc::Texture2D(width, height, FLAG_ALLOW_RENDER_TARGET, STATE_PIXEL_SHADER_RESOURCE, format::R16G16_FLOAT);
        velocityBufferDesc.mClearValue = { .mFormat = format::R16G16_FLOAT,.mColor = {0,0,0,0} , .mStencil = 0, .mIsDepth = false };
        GPUResourceDesc worldPosBufferDesc = GPUResourceDesc::Texture2D(width, height, FLAG_ALLOW_RENDER_TARGET, STATE_PIXEL_SHADER_RESOURCE, format::R16G16B16A16_FLOAT); // Higher precision for world pos
        worldPosBufferDesc.mClearValue = worldPosClear;
        GPUResourceDesc depthBufferDesc = GPUResourceDesc::Texture2D(width, height, FLAG_ALLOW_DEPTH, STATE_PIXEL_SHADER_RESOURCE, format::D32_FLOAT);
        depthBufferDesc.mClearValue = { .mFormat = format::D32_FLOAT,.mColor = {1.f,0,0,0} , .mStencil = 0, .mIsDepth = true };

        mGBufferA = gResourceManager->CreateResource(materialBufferDesc, ID("GBuffer/GBufferMatA"));
        mGBufferB = gResourceManager->CreateResource(materialBufferDesc, ID("GBuffer/GBufferMatB"));
        mGBufferC = gResourceManager->CreateResource(worldPosBufferDesc, ID("GBuffer/GBufferWorldPos"));
        mGBufferD = gResourceManager->CreateResource(velocityBufferDesc, ID("GBuffer/GBufferVelocityBuffer"));
        mDepthBuffer = gResourceManager->CreateResource(depthBufferDesc, ID("GBuffer/GBufferDepth"));

        TextureDesc gBufferDesc = { .mUsage = tex::USAGE_RENDER_TARGET, .mBuffer = mGBufferA };
        mGBufferRenderTargets[0] = gResourceManager->CreateTexture(gBufferDesc, ID("GBuffer/GBufferTargetA"));
        gBufferDesc.mBuffer = mGBufferB;
        mGBufferRenderTargets[1] = gResourceManager->CreateTexture(gBufferDesc, ID("GBuffer/GBufferTargetB"));
        gBufferDesc.mBuffer = mGBufferC;
        mGBufferRenderTargets[2] = gResourceManager->CreateTexture(gBufferDesc, ID("GBuffer/GBufferTargetC"));
        gBufferDesc.mBuffer = mGBufferD;
        mGBufferRenderTargets[3] = gResourceManager->CreateTexture(gBufferDesc, ID("GBuffer/GBufferTargetD"));

        gBufferDesc.mUsage = tex::USAGE_DEPTH_STENCIL;
        gBufferDesc.mBuffer = mDepthBuffer;
        mDepthTarget = gResourceManager->CreateTexture(gBufferDesc, ID("GBuffer/GBufferDepthTarget"));

        mGBufferPSO = CreateGBufferPSO({ format::R8G8B8A8_UNORM, format::R8G8B8A8_UNORM, format::R16G16B16A16_FLOAT, format::R16G16_FLOAT }, format::D32_FLOAT);

        TextureDesc gBufferTexDesc = { .mUsage = tex::USAGE_SHADER, .mBuffer = mGBufferA };
        gResourceManager->CreateTexture(gBufferTexDesc, ID("GBuffer/GBufferMatA_SRV"));
        gBufferTexDesc.mBuffer = mGBufferB;
        gResourceManager->CreateTexture(gBufferTexDesc, ID("GBuffer/GBufferMatB_SRV"));
        gBufferTexDesc.mBuffer = mGBufferC;
        gResourceManager->CreateTexture(gBufferTexDesc, ID("GBuffer/GBufferWorldPos_SRV"));
        gBufferTexDesc.mBuffer = mDepthBuffer;
        gBufferTexDesc.mFormat = format::R32_FLOAT;
        gResourceManager->CreateTexture(gBufferTexDesc, ID("GBuffer/GBufferDepth_SRV"));
        gBufferTexDesc.mBuffer = mGBufferD;
        gBufferTexDesc.mFormat = format::R16G16_FLOAT;
        gResourceManager->CreateTexture(gBufferTexDesc, ID("GBuffer/GBufferVelocity_SRV"));
    }

    void GBuffer::Execute(const RenderPassData& renderPassData)
    {
        auto ctx = gRenderer->GetContext();
        const auto bindAndDrawObject = [&](ConstantBufferView objCb, ConstantBufferView matCb, Mesh* mesh)
        {
            ctx->BindConstantBuffer(0, (uint32_t)objCb.mHeapIndex); // Bind object data
            ctx->SetMesh(mesh);
            for (auto entry : mesh->GetDesc().mMeshEntries)
            {
                ctx->DrawIndexedInstanced(entry.mNumIndices, 1, entry.mBaseIndex, entry.mBaseVertex, 0);
            }
        };

        NV_GPU_EVENT("GBuffer", ((ContextDX12*)ctx)->GetCommandList());

        // Transition buffers to render target state
        TransitionBarrier barriers[] = {
			{.mTo = STATE_RENDER_TARGET, .mResource = mGBufferA,  },
            {.mTo = STATE_RENDER_TARGET, .mResource = mGBufferB,  },
            {.mTo = STATE_RENDER_TARGET, .mResource = mGBufferC,  },
            {.mTo = STATE_RENDER_TARGET, .mResource = mGBufferD,  },
			{.mTo = STATE_DEPTH_WRITE,   .mResource = mDepthBuffer }
		};

        ctx->ResourceBarrier({ &barriers[0], _countof(barriers) });

        SetContextDefault(ctx);
        Rect rect = {};
        rect.mLeft = 0;
        rect.mTop = 0;
        rect.mRight = gWindow->GetWidth();
        rect.mBottom = gWindow->GetHeight();

        ctx->ClearDepthStencil(mDepthTarget, 1.f, 0, 1, &rect);
        for (auto rt : mGBufferRenderTargets)
        {
            float clearColor[] = { 0, 0, 0, 0 };
            ctx->ClearRenderTarget(rt, clearColor, 1, &rect);
        }

        ctx->SetRenderTarget({ mGBufferRenderTargets, _countof(mGBufferRenderTargets)}, mDepthTarget);
        ctx->SetPipeline(mGBufferPSO);
        ctx->BindConstantBuffer(3, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);

        auto objectCbs = renderPassData.mRenderDataArray.GetObjectDescriptors();
        auto materialCbs = renderPassData.mRenderDataArray.GetMaterialDescriptors();
        auto boneCbs = renderPassData.mRenderDataArray.GetBoneDescriptors();
        for (size_t i = 0; i < objectCbs.Size(); ++i)
        {
            const auto& objectCb = objectCbs[i];
            const auto& matCb = materialCbs[i];
            const auto mesh = renderPassData.mRenderData[i].mpMesh;
            if (mesh)
            {
                if (mesh->HasBones())
                {
                    // TODO: Not implemented for GBuffer yet
                }
                else
                {
                    bindAndDrawObject(objectCb, matCb, mesh); // Increments bonesCbIdx
                }
            }
        }

        // Transition buffers back to pixel shader resource state
        TransitionBarrier endBarriers[] = {
            {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = mGBufferA,  },
            {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = mGBufferB,  },
            {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = mGBufferC,  },
            {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = mGBufferD,  },
            {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = mDepthBuffer }
        };

        ctx->ResourceBarrier({ &endBarriers[0], _countof(endBarriers) });
    }

    void GBuffer::Destroy()
    {
    }
}

