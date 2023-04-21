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

    static Handle<Texture> testTex;
    static void BuildBVHs()
    {
        nv::Vector<TriangulatedMesh> triMeshes;
        nv::Vector<graphics::BVHInstance> bvhInstances;
        nv::Vector<Handle<Entity>> entities;
        nv::Vector<graphics::AABB> bvhAabbs;
        TLAS tlas;
        uint32_t index = 0;

        // Create BVH Instances for all drawable entities, store in component - Done
        // Build TLAS with all BVH instances - Done
        // Create structured buffer of bvh nodes in each BVHData, store index of BVHData in BVH Instance
        // Create structured buffer of BVH instances
        // Create structured buffer of TLAS
        gEntityManager.GetEntities(entities);

        const auto getAabb = [](const float3& bmax, const float3& bmin, const float4x4& mat)
        {
            auto bounds = bvh::AABB();
            for (int32_t i = 0; i < 8; ++i)
            {
                auto position = float3(i & 1 ? bmax.x : bmin.x,
                    i & 2 ? bmax.y : bmin.y, i & 4 ? bmax.z : bmin.z);
                auto pos = Load(position);
                auto transform = Load(mat);
                pos = math::Vector3Transform(pos, transform);
                Store(pos, position);
                bounds.Grow(position);
            }

            return bounds.GfxAABB();
        };

        for (auto handle : entities)
        {
            Entity* pEntity = gEntityManager.GetEntity(handle);
            if (pEntity->Has<Renderable>())
            {
                Renderable* renderable = pEntity->Get<Renderable>();
                auto pMesh = gResourceManager->GetMesh(renderable->mMesh);
                if (!pMesh)
                    continue;

                auto& bvh = pMesh->GetBVH();
                if(bvh.mBvhNodes.empty())
                    BuildBVH(pMesh, bvh, triMeshes.Emplace());

                TransformRef transform = pEntity->GetTransform();
                float4x4 mat = transform.GetTransformMatrixTransposed();
                graphics::BVHInstance& bvhInstance = bvhInstances.Emplace();
                auto& aabb = bvhAabbs.Emplace();
                aabb = getAabb(bvh.mBvhNodes[0].AABBMax, bvh.mBvhNodes[0].AABBMin, transform.GetTransformMatrix());
                
                bvhInstance.transform = mat;
                bvhInstance.invTransform = transform.GetTransformMatrixInverseTransposed();
                bvhInstance.idx = index;
                index++;
            }
        }

        if(bvhInstances.size() > 0)
            BuildTLAS(bvhInstances.Span(), bvhAabbs.Span(), tlas);

        uint32_t test[] = { 1, 2, 3, 4 };
        Handle<GPUResource> buffer;
        testTex = CreateStructuredBuffer(sizeof(uint32_t), 4, buffer);

        auto res = gResourceManager->GetGPUResource(buffer);

        res->MapMemory();
        res->UploadMapped((uint8_t*)test, sizeof(test), 0);
    }

    void RTCompute::Execute(const RenderPassData& renderPassData)
    {
        static bool done = false;
        if (!done && renderPassData.mRenderData.mSize > 0)
        {
            BuildBVHs();
            done = true;
        }

        auto skyHandle = gResourceManager->GetTextureHandle(ID("Textures/Sky.hdr"));
        auto ctx = gRenderer->GetContext();
        SetComputeDefault(ctx);
        
        auto structTestTex = gResourceManager->GetTexture(testTex);
        TraceParams params = 
        {
            .Resolution = float2((float)gWindow->GetWidth(), (float)gWindow->GetHeight()),
            .ScaleFactor = 1.f / SCALE,
            .StructBufferIdx = structTestTex? structTestTex->GetHeapIndex() : 0
        };

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

