#include "pch.h"
#include "RTCompute.h"

#include <Renderer/CommonDefines.h>
#include <Renderer/PipelineState.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Shader.h>
#include <Renderer/Window.h>
#include <Renderer/GPUResource.h>
#include <Renderer/RenderDataArray.h>
#include <Renderer/ConstantBufferPool.h>
#include <BVH/BVH.h>

#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Asset.h>

namespace nv::graphics
{
    using namespace components;
    using namespace asset;
    using namespace ecs;
    using namespace buffer;
    using namespace bvh;

    struct RTComputeObjects
    {
        Handle<GPUResource> mOutputBuffer;
        Handle<Texture>     mOutputUAV;
        ConstantBufferView  mTraceParamsCBV;
    };

    static RTComputeObjects sRTComputeObjects;
    constexpr uint32_t SCALE = 2;

    void RTCompute::Init()
    {
        auto cs = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/RayTraceCS.hlsl") };
        auto rtcs = gResourceManager->CreateShader({ cs, shader::COMPUTE });
        PipelineStateDesc psoDesc = { .mPipelineType = PIPELINE_COMPUTE, .mCS = rtcs };
        mRTComputePSO = gResourceManager->CreatePipelineState(psoDesc);

        uint32_t height = gWindow->GetHeight() / SCALE;
        uint32_t width = gWindow->GetWidth() / SCALE;

        GPUResourceDesc desc =
        {
            .mWidth = width,
            .mHeight = height,
            .mFormat = format::R8G8B8A8_UNORM,
            .mType = buffer::TYPE_TEXTURE_2D,
            .mFlags = FLAG_ALLOW_UNORDERED,
            .mInitialState = STATE_COPY_SOURCE,
            .mMipLevels = 1,
            .mSampleCount = 1
        };

        auto outputBuffer = gResourceManager->CreateResource(desc, ID("RTPass/OutputBuffer"));
        sRTComputeObjects.mOutputBuffer = outputBuffer;
        TextureDesc texDesc =
        {
            .mUsage = tex::USAGE_UNORDERED,
            .mBuffer = outputBuffer,
            .mType = tex::TEXTURE_2D,
            .mUseRayTracingHeap = false
        };

        auto outputBufferTexture = gResourceManager->CreateTexture(texDesc, ID("RTPass/OutputBufferTex"));
        sRTComputeObjects.mOutputUAV = outputBufferTexture;
        sRTComputeObjects.mTraceParamsCBV = gpConstantBufferPool->GetConstantBuffer<TraceParams>();
    }

    static void BuildBVHs(nv::Span<Mesh*> meshes)
    {
        for (auto pMesh : meshes)
        {
            if (pMesh)
            {
                BuildBVH(pMesh, pMesh->GetBVH());
                // Create BVH Instances for all drawable entities, store in component.
                // Build TLAS with all BVH instances
                // Create structured buffer of bvh nodes in each BVHData, store index of BVHData in BVH Instance
                // Create structured buffer of BVH instances
                // Create structured buffer of TLAS
            }
        }
    }

    void RTCompute::Execute(const RenderPassData& renderPassData)
    {
        static bool done = false;
        if (!done && renderPassData.mRenderData.mSize > 0)
        {
            nv::Span<Mesh*> meshes{ renderPassData.mRenderData.mppMeshes, renderPassData.mRenderData.mSize };
            BuildBVHs(meshes);
            done = true;
        }

        auto skyHandle = gResourceManager->GetTextureHandle(ID("Textures/Sky.hdr"));
        auto ctx = gRenderer->GetContext();
        SetComputeDefault(ctx);
        
        TraceParams params = { .Resolution = float2((float)gWindow->GetWidth(), (float)gWindow->GetHeight()), .ScaleFactor = 1.f / SCALE };
        gRenderer->UploadToConstantBuffer(sRTComputeObjects.mTraceParamsCBV, (uint8_t*)&params, (uint32_t)sizeof(params));

        TransitionBarrier initBarriers[] = { {.mTo = STATE_COPY_DEST, .mResource = sRTComputeObjects.mOutputBuffer } };
        ctx->ResourceBarrier({ &initBarriers[0] , ArrayCountOf(initBarriers) });
        
        ctx->SetPipeline(mRTComputePSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mTraceParamsCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTexture(2, sRTComputeObjects.mOutputUAV);
        ctx->ComputeBindTexture(3, skyHandle);

        ctx->Dispatch(gWindow->GetWidth() / SCALE, gWindow->GetHeight() / SCALE, 1);

        TransitionBarrier endBarriers[] = { {.mTo = STATE_COMMON, .mResource = sRTComputeObjects.mOutputBuffer } };
        ctx->ResourceBarrier({ &endBarriers[0] , ArrayCountOf(endBarriers) });

        //SetContextDefault(ctx);
    }

    void RTCompute::Destroy()
    {
    }
}

