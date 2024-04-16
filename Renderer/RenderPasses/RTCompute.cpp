#include "pch.h"
#include "RTCompute.h"

#define NV_CUSTOM_RAYTRACING 0

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
#include <Renderer/GlobalRenderSettings.h>
#include <BVH/BVH.h>

#include <Components/Renderable.h>
#include <Engine/EntityComponent.h>
#include <Asset.h>

#if !NV_CUSTOM_RAYTRACING
#include <DX12/RayTracingHelpers.h>
#include <DX12/ContextDX12.h>
#endif

namespace nv::graphics
{
    using namespace components;
    using namespace asset;
    using namespace ecs;
    using namespace buffer;
    using namespace bvh;

    constexpr uint32_t SCALE = 1;
    constexpr uint32_t MAX_OBJECTS = 32;

    Handle<Texture> CreateStructuredBuffer(uint32_t stride, uint32_t elemCount, Handle<GPUResource>& buffer)
    {
        const uint32_t bufferSize = stride * elemCount;
        GPUResourceDesc desc = GPUResourceDesc::UploadConstBuffer(bufferSize);

        buffer = gResourceManager->CreateResource(desc);
        TextureDesc texDesc =
        {
            .mUsage = tex::USAGE_SHADER,
            .mBuffer = buffer,
            .mType = tex::Type::BUFFER,
            .mBufferData =
            {
                .mNumElements = elemCount,
                .mStructureByteStride = stride
            }
        };

        return gResourceManager->CreateTexture(texDesc);
    }

    template<typename T>
    Handle<Texture> CreateAndUpload(nv::Span<T> data, Handle<GPUResource>& buffer)
    {
        constexpr uint32_t strideSize = (uint32_t)sizeof(T);
        const uint32_t elemCount = (uint32_t)data.Size();
        const uint32_t bufferSize = strideSize * elemCount;

        Handle<Texture> texHandle = CreateStructuredBuffer(strideSize, elemCount, buffer);
        auto res = gResourceManager->GetGPUResource(buffer);

        res->MapMemory();
        res->UploadMapped((uint8_t*)data.mData, bufferSize, 0);
        return texHandle;
    }

    template <typename T>
    void UploadToConstantBuffer(const ConstantBufferView& view, const T& data)
    {
        gRenderer->UploadToConstantBuffer(view, (uint8_t*)&data, (uint32_t)sizeof(data));
    }

    static Handle<PipelineState> CreateComputePSO(std::string_view shaderPath)
    {
        auto cs = asset::AssetID{ asset::ASSET_SHADER, ID(shaderPath.data()) };
        auto rtcs = gResourceManager->CreateShader({ cs, shader::COMPUTE });
        PipelineStateDesc psoDesc = { .mPipelineType = PIPELINE_COMPUTE, .mCS = rtcs };
        return gResourceManager->CreatePipelineState(psoDesc);
    }


    struct RTComputeObjects
    {
        Handle<GPUResource> mOutputBuffer;
        Handle<GPUResource> mAccumBuffer;
        Handle<GPUResource> mPrevAccumBuffer;
        Handle<GPUResource> mDirectLightBuffer;
        Handle<Texture>     mOutputUAV;
        Handle<Texture>	    mAccumUAV;
        Handle<Texture>     mPrevAccumUAV;
        Handle<Texture>     mDirectLightUAV;
        Handle<Texture>     mDepthTexture;
        ConstantBufferView  mTraceParamsCBV;
        ConstantBufferView  mTraceAccumCBV;
        ConstantBufferView  mBlurParamsCBV;
    };

    static Handle<GPUResource> meshInstanceBuffer;
    static Handle<Texture> meshInstanceData;
    static RTComputeObjects sRTComputeObjects;
    static dx12::RayTracingRuntimeData sRTRuntimeData;

    void RTCompute::Init()
    {
        mRTDirectLightingPSO = CreateComputePSO("Shaders/DirectLightingRTCS.hlsl");
        mDirectGIRayTracePSO = CreateComputePSO("Shaders/RayTraceCS.hlsl");
        mGIAccumulatePSO = CreateComputePSO("Shaders/AccumulateRTCS.hlsl");
        mBlurPSO = CreateComputePSO("Shaders/BlurCS.hlsl");

        uint32_t height = gWindow->GetHeight() / SCALE;
        uint32_t width = gWindow->GetWidth() / SCALE;

        GPUResourceDesc desc = GPUResourceDesc::Texture2D(width, height, FLAG_ALLOW_UNORDERED, STATE_COMMON);

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
        sRTComputeObjects.mTraceAccumCBV = gpConstantBufferPool->GetConstantBuffer<TraceAccumParams>();
        sRTComputeObjects.mBlurParamsCBV = gpConstantBufferPool->GetConstantBuffer<BlurParams>();

        sRTComputeObjects.mAccumBuffer = gResourceManager->CreateResource(desc, ID("RTPass/LightAccumBuffer"));
        texDesc.mBuffer = sRTComputeObjects.mAccumBuffer;
        sRTComputeObjects.mAccumUAV = gResourceManager->CreateTexture(texDesc, ID("RTPass/LightAccumBufferTex"));

        sRTComputeObjects.mPrevAccumBuffer = gResourceManager->CreateResource(desc, ID("RTPass/PrevAccumBuffer"));
        texDesc.mBuffer = sRTComputeObjects.mPrevAccumBuffer;
        sRTComputeObjects.mPrevAccumUAV = gResourceManager->CreateTexture(texDesc, ID("RTPass/PrevAccumBufferTex"));

        desc.mInitialState = STATE_UNORDERED_ACCESS;
        sRTComputeObjects.mDirectLightBuffer = gResourceManager->CreateResource(desc, ID("RTPass/DirectLightBuffer"));
        texDesc.mBuffer = sRTComputeObjects.mDirectLightBuffer;
        sRTComputeObjects.mDirectLightUAV = gResourceManager->CreateTexture(texDesc, ID("RTPass/DirectLightTex"));

        TextureDesc lightAccumSrvDesc = 
        {
            .mUsage = tex::USAGE_SHADER,
            .mBuffer = sRTComputeObjects.mOutputBuffer,
            .mType = tex::Type::TEXTURE_2D
        };

        gResourceManager->CreateTexture(lightAccumSrvDesc, ID("RTPass/LightAccumBufferSRV"));

        // Get Depth Texture and transition buffer to pixel shader resource
        auto depthTex = gRenderer->GetDefaultDepthTarget();
        auto depthResource = gResourceManager->GetTexture(depthTex)->GetBuffer();
        TextureDesc depthDesc =
        {
            .mUsage = tex::USAGE_SHADER,
            .mFormat = format::R32_FLOAT,
			.mBuffer = depthResource,
			.mType = tex::Type::TEXTURE_2D,
        };

        sRTComputeObjects.mDepthTexture = gResourceManager->CreateTexture(depthDesc, ID("RTPass/DepthTexture"));
        

        meshInstanceData = CreateStructuredBuffer(sizeof(MeshInstanceData), MAX_OBJECTS, meshInstanceBuffer);
        auto pResource = gResourceManager->GetGPUResource(meshInstanceBuffer);
        pResource->MapMemory(); // Keep memory mapped
    }

    void RTCompute::Execute(const RenderPassData& renderPassData)
    {
        static std::vector<uint32_t> meshIds = {};
        static std::vector<MeshInstanceData> rtMeshData = {};
        static uint32_t sFrameCounter = 0;
        static bool sbBuildAccelerationStructure = true;

        constexpr uint32_t BVH_REBUILD_THRESHOLD_FRAMES = 512;

        if (sFrameCounter > BVH_REBUILD_THRESHOLD_FRAMES)
            sbBuildAccelerationStructure = false; // DISABLED

        sFrameCounter++;
        auto ctx = gRenderer->GetContext();

        if (sbBuildAccelerationStructure && renderPassData.mRenderData.mSize > 0)
        {
            meshIds.clear();
            rtMeshData.clear();

            std::vector<Mesh*> meshes;
            std::vector<float4x4> transforms;
            for (uint32_t i = 0; i < renderPassData.mRenderData.mSize; ++i)
            {
                Mesh* pMesh = renderPassData.mRenderData.mppMeshes[i];
                if (pMesh)
                {
                    if (pMesh->HasBones()) continue; // Animated Mesh not supported yet.
                    meshIds.push_back(i);
                    auto mesh = ((MeshDX12*)pMesh);
                    mesh->GenerateBufferSRVs();
                    meshes.push_back(pMesh);
                    transforms.push_back(renderPassData.mRenderData.mpObjectData[i].World);
                }
            }

            dx12::BuildAccelerationStructure(((ContextDX12*)ctx)->GetDXRCommandList(), meshes, transforms, sRTRuntimeData);
            sbBuildAccelerationStructure = false;
            sFrameCounter = 0;
        }
        else
        {
            // Update acceleration structures
            constexpr bool UPDATE = true;

            std::vector<Mesh*> meshes; 
            std::vector<float4x4> transforms;
            for (const auto meshId : meshIds)
            {
                auto pMesh = (MeshDX12*)renderPassData.mRenderData.mppMeshes[meshId];
                meshes.push_back(pMesh);
                transforms.push_back(renderPassData.mRenderData.mpObjectData[meshId].World);
            }

            dx12::BuildAccelerationStructure(((ContextDX12*)ctx)->GetDXRCommandList(), meshes, transforms, sRTRuntimeData, UPDATE);
        }

        assert(meshIds.size() <= MAX_OBJECTS);

        if (!meshIds.empty())
        {
            rtMeshData.clear();
            for (const uint32_t meshId : meshIds)
            {
                const auto* pMesh = (MeshDX12*)renderPassData.mRenderData.mppMeshes[meshId];
                const auto vbTex = pMesh->GetVertexBufferSRVHandle();
                const auto ibTex = pMesh->GetIndexBufferSRVHandle();

                const auto objectCBV = renderPassData.mRenderDataArray.GetObjectDescriptors()[meshId];
                rtMeshData.push_back(
                {
                    .ObjectDataIdx = (uint32_t)objectCBV.mHeapIndex,
                    .VertexBufferIdx = vbTex ? gResourceManager->GetTexture(vbTex)->GetHeapIndex() : 0,
                    .IndexBufferIdx = ibTex ? gResourceManager->GetTexture(ibTex)->GetHeapIndex() : 0,
                });
            }

            auto pResource = gResourceManager->GetGPUResource(meshInstanceBuffer);
            pResource->UploadMapped((uint8_t*)rtMeshData.data(), rtMeshData.size() * sizeof(MeshInstanceData));
        }

        auto skyHandle = gComponentManager.GetComponents<SkyboxComponent>()[0].mSkybox; // gResourceManager->GetTextureHandle(ID("Textures/Sky.hdr"));

        SetComputeDefault(ctx);
        
        auto tlasHandle = gResourceManager->GetTextureHandle(ID("RT/TlasSRV"));

        auto structTestTex = gResourceManager->GetTexture(meshInstanceData);
       
        static uint32_t frameCount = 0;
        frameCount++;
        TraceParams params = 
        {
            .Resolution = float2((float)gWindow->GetWidth(), (float)gWindow->GetHeight()),
            .ScaleFactor = 1.f / SCALE,
            .FrameCount = frameCount,
            .RTSceneIdx = gResourceManager->GetTexture(tlasHandle)->GetHeapIndex(),
            .SkyBoxHandle = gResourceManager->GetTexture(skyHandle)->GetHeapIndex(),
            .EnableShadows = gRenderSettings.mbEnableRTShadows,
            .EnableIndirectGI = gRenderSettings.mbEnableRTDiffuseGI
        }; 
        
        gRenderer->UploadToConstantBuffer(sRTComputeObjects.mTraceParamsCBV, (uint8_t*)&params, (uint32_t)sizeof(params));

        TraceAccumParams accumParams =
        {
            .AccumulationAlpha = 0.1f,
            .PrevFrameTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mPrevAccumUAV)->GetHeapIndex(),
            .AccumulationTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mAccumUAV)->GetHeapIndex(),
            .FrameIndex = frameCount
        };

        gRenderer->UploadToConstantBuffer(sRTComputeObjects.mTraceAccumCBV, (uint8_t*)&accumParams, (uint32_t)sizeof(accumParams));

        auto depthResource = gResourceManager->GetTexture(sRTComputeObjects.mDepthTexture)->GetBuffer();

        TransitionBarrier initBarriers[] = { 
            {.mTo = STATE_COPY_DEST, .mResource = sRTComputeObjects.mOutputBuffer },
			{.mTo = STATE_COPY_SOURCE, .mResource = sRTComputeObjects.mAccumBuffer },
			{.mTo = STATE_COPY_DEST, .mResource = sRTComputeObjects.mPrevAccumBuffer },
            {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = depthResource }
        };

        ctx->ResourceBarrier({ &initBarriers[0] , ArrayCountOf(initBarriers) });
        ctx->CopyResource(sRTComputeObjects.mPrevAccumBuffer, sRTComputeObjects.mAccumBuffer);
        
        // Resource Barriers for RT Compute
        TransitionBarrier rtInitBarriers[] = {
            {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mAccumBuffer },
            {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mPrevAccumBuffer }
        };

        constexpr uint32_t DISPATCH_SCALE = 8;

        ctx->ResourceBarrier({ &rtInitBarriers[0] , ArrayCountOf(rtInitBarriers) });

        // Direct Diffuse GI
        ctx->SetPipeline(mDirectGIRayTracePSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mTraceParamsCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTexture(2, sRTComputeObjects.mOutputUAV);
        ctx->ComputeBindTexture(3, meshInstanceData);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        // Accumulate GI Results
        ctx->SetPipeline(mGIAccumulatePSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mTraceAccumCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTexture(2, sRTComputeObjects.mOutputUAV);
        ctx->ComputeBindTexture(3, sRTComputeObjects.mDepthTexture);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        TransitionBarrier endBarriers[] = { 
            {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mOutputBuffer } ,
            {.mTo = STATE_DEPTH_WRITE, .mResource = depthResource }
        };

        ctx->ResourceBarrier({ &endBarriers[0] , ArrayCountOf(endBarriers) });

        UploadToConstantBuffer<BlurParams>(sRTComputeObjects.mBlurParamsCBV, {
            .BlurRadius = 2,
            .BlurDepthThreshold = 0.1f,
            .InputTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mAccumUAV)->GetHeapIndex()
        });

        // Blur GI Accumulation Buffer
        ctx->SetPipeline(mBlurPSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mBlurParamsCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTexture(2, sRTComputeObjects.mOutputUAV);
        ctx->ComputeBindTexture(3, sRTComputeObjects.mDepthTexture);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        // Direct Lighting
        ctx->ResourceBarrier({ UAVBarrier{.mResource = sRTComputeObjects.mOutputBuffer } });
        ctx->SetPipeline(mRTDirectLightingPSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mTraceParamsCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTextures(2, { sRTComputeObjects.mDirectLightUAV, sRTComputeObjects.mOutputUAV });
        ctx->ComputeBindTexture(3, meshInstanceData);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        {
            TransitionBarrier endBarriers[] = { 
                {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = sRTComputeObjects.mOutputBuffer } ,
            };
            ctx->ResourceBarrier({ &endBarriers[0] , ArrayCountOf(endBarriers) });
        }
    }

    void RTCompute::Destroy()
    {
    }
}

