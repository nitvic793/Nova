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
        Handle<GPUResource> mPrevNormalsBuffer;
        Handle<GPUResource> mHistoryLengthBuffer;
        Handle<GPUResource> mMeshIDBuffer;
        Handle<GPUResource> mPrevMeshIDBuffer;
        Handle<Texture>     mOutputUAV;
        Handle<Texture>	    mAccumUAV;
        Handle<Texture>     mPrevAccumUAV;
        Handle<Texture>     mDirectLightUAV;
        Handle<Texture>     mDepthTexture;
        Handle<Texture>     mPrevNormalsTex;
        Handle<Texture>     mHistoryLengthTex;
        Handle<Texture>     mMeshIDTex;
        Handle<Texture>     mPrevMeshIDTex;
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

        GPUResourceDesc desc = GPUResourceDesc::Texture2D(width, height, FLAG_ALLOW_UNORDERED, STATE_COMMON/*, format::R16G16B16A16_FLOAT*/);

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

        sRTComputeObjects.mPrevNormalsBuffer = gResourceManager->CreateResource(desc, ID("RTPass/PrevNormals"));
        texDesc.mBuffer = sRTComputeObjects.mPrevNormalsBuffer;
        sRTComputeObjects.mPrevNormalsTex = gResourceManager->CreateTexture(texDesc, ID("RTPass/PrevNormalsTex"));

        desc.mInitialState = STATE_UNORDERED_ACCESS;
        sRTComputeObjects.mDirectLightBuffer = gResourceManager->CreateResource(desc, ID("RTPass/DirectLightBuffer"));
        texDesc.mBuffer = sRTComputeObjects.mDirectLightBuffer;
        sRTComputeObjects.mDirectLightUAV = gResourceManager->CreateTexture(texDesc, ID("RTPass/DirectLightTex"));

        desc.mFormat = format::R16_FLOAT;
        sRTComputeObjects.mHistoryLengthBuffer = gResourceManager->CreateResource(desc, ID("RTPass/HistoryLengthBuffer"));
        texDesc.mBuffer = sRTComputeObjects.mHistoryLengthBuffer;
        sRTComputeObjects.mHistoryLengthTex = gResourceManager->CreateTexture(texDesc, ID("RTPass/HistoryLengthTex"));

        desc.mFormat = format::R16_UINT;
        sRTComputeObjects.mMeshIDBuffer = gResourceManager->CreateResource(desc, ID("RTPass/MeshID"));
        sRTComputeObjects.mPrevMeshIDBuffer = gResourceManager->CreateResource(desc, ID("RTPass/PrevMeshID"));
        texDesc.mBuffer = sRTComputeObjects.mMeshIDBuffer;
        sRTComputeObjects.mMeshIDTex = gResourceManager->CreateTexture(texDesc, ID("RTPass/MeshIDTex"));
        texDesc.mBuffer = sRTComputeObjects.mPrevMeshIDBuffer;
        sRTComputeObjects.mPrevMeshIDTex = gResourceManager->CreateTexture(texDesc, ID("RTPass/PrevMeshIDTex"));

        TextureDesc lightAccumSrvDesc = 
        {
            .mUsage = tex::USAGE_SHADER,
            .mBuffer = sRTComputeObjects.mOutputBuffer,
            .mType = tex::Type::TEXTURE_2D
        };

        gResourceManager->CreateTexture(lightAccumSrvDesc, ID("RTPass/LightAccumBufferSRV"));

        // Get Depth Texture and transition buffer to pixel shader resource
        sRTComputeObjects.mDepthTexture = gResourceManager->GetTextureHandle(ID("GBuffer/GBufferDepth_SRV"));// = gResourceManager->CreateTexture(depthDesc, ID("RTPass/DepthTexture"));
        

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
        static uint32_t sMeshCount = 0;

        // Rebuild only when mesh count changes
        if(sMeshCount != meshIds.size())
            sbBuildAccelerationStructure = true;

        sMeshCount = 0;
        for (uint32_t i = 0; i < renderPassData.mRenderData.mSize; ++i)
        {
            Mesh* pMesh = renderPassData.mRenderData.mppMeshes[i];
            if (pMesh)
            {
				if (pMesh->HasBones()) continue; // Animated Mesh not supported yet.
				sMeshCount++;
			}
        }

        constexpr uint32_t BVH_REBUILD_THRESHOLD_FRAMES = 512;

        //if (sFrameCounter > BVH_REBUILD_THRESHOLD_FRAMES)
        //    sbBuildAccelerationStructure = true; // DISABLED

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
            .NoiseTexIdx = gResourceManager->GetTexture(ID("Textures/bluenoise256.png"))->GetHeapIndex(),
            .FrameCount = frameCount,
            .RTSceneIdx = gResourceManager->GetTexture(tlasHandle)->GetHeapIndex(),
            .SkyBoxHandle = gResourceManager->GetTexture(skyHandle)->GetHeapIndex(),
            .EnableShadows = gRenderSettings.mbEnableRTShadows,
            .EnableIndirectGI = gRenderSettings.mbEnableRTDiffuseGI,
            .MeshIDTex = gResourceManager->GetTexture(sRTComputeObjects.mMeshIDTex)->GetHeapIndex()
        }; 
        
        gRenderer->UploadToConstantBuffer(sRTComputeObjects.mTraceParamsCBV, (uint8_t*)&params, (uint32_t)sizeof(params));

        TraceAccumParams accumParams =
        {
            .AccumulationAlpha = 0.1f,
            .PrevFrameTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mPrevAccumUAV)->GetHeapIndex(),
            .AccumulationTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mAccumUAV)->GetHeapIndex(), // GI Accum Step outputs to this texture
            .FrameIndex = frameCount,
            .PrevNormalTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mPrevNormalsTex)->GetHeapIndex(),
            .HistoryTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mHistoryLengthTex)->GetHeapIndex(),
            .MeshIDTex = gResourceManager->GetTexture(sRTComputeObjects.mMeshIDTex)->GetHeapIndex(),
            .PrevMeshIDTex = gResourceManager->GetTexture(sRTComputeObjects.mPrevMeshIDTex)->GetHeapIndex()
        };

        gRenderer->UploadToConstantBuffer(sRTComputeObjects.mTraceAccumCBV, (uint8_t*)&accumParams, (uint32_t)sizeof(accumParams));

        TransitionBarrier initBarriers[] = { 
            {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mOutputBuffer },
			{.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mAccumBuffer },
            {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mPrevNormalsBuffer},
            {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mMeshIDBuffer },
            {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mPrevMeshIDBuffer }
        };

        ctx->ResourceBarrier({ &initBarriers[0] , ArrayCountOf(initBarriers) });
        
        constexpr uint32_t DISPATCH_SCALE = 8;

        // Direct Diffuse GI
        ctx->SetPipeline(mDirectGIRayTracePSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mTraceParamsCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTexture(2, sRTComputeObjects.mOutputUAV); // Outputs to this
        ctx->ComputeBindTexture(3, meshInstanceData);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        ctx->ResourceBarrier({ UAVBarrier{.mResource = sRTComputeObjects.mOutputBuffer } });
      

        // Accumulate GI Results
        ctx->SetPipeline(mGIAccumulatePSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mTraceAccumCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTexture(2, sRTComputeObjects.mOutputUAV);
        ctx->ComputeBindTexture(3, sRTComputeObjects.mDepthTexture);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        ctx->ResourceBarrier({ UAVBarrier{.mResource = sRTComputeObjects.mAccumBuffer } });

        UploadToConstantBuffer<BlurParams>(sRTComputeObjects.mBlurParamsCBV, {
            .BlurRadius = 4,
            .BlurDepthThreshold = 0.1f,
            .InputTexIdx = gResourceManager->GetTexture(sRTComputeObjects.mAccumUAV)->GetHeapIndex()
        });

        // TODO: Confirm order, Blur before or after GI Accumulation
        // Blur GI Accumulation Buffer
        ctx->SetPipeline(mBlurPSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mBlurParamsCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTexture(2, sRTComputeObjects.mOutputUAV); // Blur outputs to this
        ctx->ComputeBindTexture(3, sRTComputeObjects.mDepthTexture);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        ctx->ResourceBarrier({ UAVBarrier{.mResource = sRTComputeObjects.mOutputBuffer } });

        {
            // Copy output buffer to previous accumulation buffer
            TransitionBarrier barriers[] = {
                {.mTo = STATE_COPY_DEST, .mResource = sRTComputeObjects.mPrevAccumBuffer },
                {.mTo = STATE_COPY_SOURCE, .mResource = sRTComputeObjects.mAccumBuffer }
            };
            ctx->ResourceBarrier({ &barriers[0] , ArrayCountOf(barriers) });
            ctx->CopyResource(sRTComputeObjects.mPrevAccumBuffer, sRTComputeObjects.mAccumBuffer);

            // Transition prev buffer to unordered access
            TransitionBarrier endBarriers[] = {
                {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mPrevAccumBuffer } ,
                {.mTo = STATE_UNORDERED_ACCESS, .mResource = sRTComputeObjects.mAccumBuffer }
            };
            ctx->ResourceBarrier({ &endBarriers[0] , ArrayCountOf(endBarriers) });
        }

        // Direct Lighting
        ctx->SetPipeline(mRTDirectLightingPSO);
        ctx->ComputeBindConstantBuffer(0, (uint32_t)sRTComputeObjects.mTraceParamsCBV.mHeapIndex);
        ctx->ComputeBindConstantBuffer(1, (uint32_t)renderPassData.mFrameDataCBV.mHeapIndex);
        ctx->ComputeBindTextures(2, { sRTComputeObjects.mDirectLightUAV, sRTComputeObjects.mOutputUAV }); // Outputs to Direct Light UAV
        ctx->ComputeBindTexture(3, meshInstanceData);
        ctx->Dispatch(gWindow->GetWidth() / DISPATCH_SCALE, gWindow->GetHeight() / DISPATCH_SCALE, 1);

        Handle<GPUResource> curNormalsBuffer = gResourceManager->GetGPUResourceHandle(ID("GBuffer/GBufferMatB"));
        {
            TransitionBarrier endBarriers[] = {
                {.mTo = STATE_PIXEL_SHADER_RESOURCE, .mResource = sRTComputeObjects.mOutputBuffer } ,
                {.mTo = STATE_COPY_DEST, .mResource = sRTComputeObjects.mPrevNormalsBuffer },
                {.mTo = STATE_COPY_SOURCE, .mResource = curNormalsBuffer },
                {.mTo = STATE_COPY_DEST, .mResource = sRTComputeObjects.mPrevMeshIDBuffer },
                {.mTo = STATE_COPY_SOURCE, .mResource = sRTComputeObjects.mMeshIDBuffer },
            };
            ctx->ResourceBarrier({ &endBarriers[0] , ArrayCountOf(endBarriers) });
        }

        // Copy current normals to previous normals buffer
        ctx->CopyResource(sRTComputeObjects.mPrevNormalsBuffer, curNormalsBuffer);
        ctx->CopyResource(sRTComputeObjects.mPrevMeshIDBuffer, sRTComputeObjects.mMeshIDBuffer);
    }

    void RTCompute::Destroy()
    {
    }
}

