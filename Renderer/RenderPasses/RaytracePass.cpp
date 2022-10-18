#include "pch.h"
#include "RaytracePass.h"
#include <DX12/NvidiaDXR/BottomLevelASGenerator.h>
#include <DX12/NvidiaDXR/TopLevelASGenerator.h>
#include <Renderer/CommonDefines.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Window.h>
#include <fmt/format.h>

#if NV_RENDERER_DX12
#include <DX12/MeshDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <D3D12MemAlloc.h>
#include <DX12/DXR.h>
#include <DX12/ContextDX12.h>
#endif

namespace nv::graphics
{
    using namespace nv_helpers_dx12;
    using namespace buffer;

#if NV_RENDERER_DX12
    void RaytracePass::Init()
    {
        CreateOutputBuffer();
    }

    bool done = false;
    void RaytracePass::Execute(const RenderPassData& renderPassData)
    {
        if(!done)
            CreateRaytracingStructures(renderPassData);
    }

    void RaytracePass::Destroy()
    {
    }

    void RaytracePass::CreateRaytracingStructures(const RenderPassData& renderPassData)
    {
        auto device = (DeviceDX12*)gRenderer->GetDevice();
        auto renderer = (RendererDX12*)gRenderer;
        auto ctx = (ContextDX12*)renderer->GetContext();

        TopLevelASGenerator topLevelASGenerator;
        AccelerationStructureBuffers tlasBuffers;

        const auto createBlas = [&](const RenderObject& renderObj, uint32_t index)
        {
            AccelerationStructureBuffers buffers;
            BottomLevelASGenerator bottomLevelAS;

            if (renderObj.mpMesh)
            {
                auto mesh = renderObj.mpMesh->As<MeshDX12>();
                bottomLevelAS.AddVertexBuffer(
                    mesh->GetVertexBuffer(), 0, (uint32_t)mesh->GetVertexCount(), sizeof(Vertex),
                    mesh->GetIndexBuffer(), 0, (uint32_t)mesh->GetIndexCount(), nullptr, 0);
            }
          
            UINT64 scratchSizeInBytes = 0;
            UINT64 resultSizeInBytes = 0;

            bottomLevelAS.ComputeASBufferSizes((ID3D12Device5*)device->GetDevice(), false, &scratchSizeInBytes,
                &resultSizeInBytes);

            GPUResourceDesc scratchDesc = { .mSize = (uint32_t)scratchSizeInBytes, .mFlags = FLAG_ALLOW_UNORDERED, .mInitialState = STATE_COMMON };
            GPUResourceDesc resultDesc = { .mSize = (uint32_t)resultSizeInBytes, .mFlags = FLAG_ALLOW_UNORDERED, .mInitialState = STATE_RAYTRACING_STRUCTURE };
            auto scratch = gResourceManager->CreateResource(scratchDesc, ID(fmt::format("RTPass/BlasScratch{}", index).c_str()));
            auto result = gResourceManager->CreateResource(resultDesc, ID(fmt::format("RTPass/BlasResult{}", index).c_str()));

            buffers.mScratch = gResourceManager->GetGPUResource(scratch)->As<GPUResourceDX12>()->GetResource();
            buffers.mResult = gResourceManager->GetGPUResource(result)->As<GPUResourceDX12>()->GetResource();

            bottomLevelAS.Generate(ctx->GetDXRCommandList(), buffers.mScratch.Get(), buffers.mResult.Get(), false, nullptr);
            return buffers;
        };

        struct Object
        {
            AccelerationStructureBuffers    mBlas;
            ObjectData                      mData;
        };

        std::vector<Object> blasList;

        const auto addToTlas = [&](Object& objData, uint32_t instanceId, uint32_t hitGroup)
        {
            auto transform = Load(objData.mData.World);
            topLevelASGenerator.AddInstance(objData.mBlas.mResult.Get(), transform, instanceId, hitGroup);
        };

        const auto createTlas = [&]()
        {
            UINT64 scratchSize, resultSize, instanceDescsSize;

            topLevelASGenerator.ComputeASBufferSizes((ID3D12Device5*)device->GetDevice(), true, &scratchSize,
                &resultSize, &instanceDescsSize);

            GPUResourceDesc scratchDesc = { .mSize = (uint32_t)scratchSize, .mFlags = FLAG_ALLOW_UNORDERED, .mInitialState = STATE_COMMON };
            GPUResourceDesc resultDesc = { .mSize = (uint32_t)resultSize, .mFlags = FLAG_ALLOW_UNORDERED, .mInitialState = STATE_RAYTRACING_STRUCTURE };
            GPUResourceDesc instanceDesc = { .mSize = (uint32_t)instanceDescsSize,.mInitialState = STATE_GENERIC_READ, .mBufferMode = BUFFER_MODE_UPLOAD };
            auto scratch = gResourceManager->CreateResource(scratchDesc, ID("RTPass/TlasScratch"));
            auto result = gResourceManager->CreateResource(resultDesc, ID("RTPass/TlasResult"));
            auto instance = gResourceManager->CreateResource(instanceDesc, ID("RTPass/TlasInstance"));

            tlasBuffers.mScratch = gResourceManager->GetGPUResource(scratch)->As<GPUResourceDX12>()->GetResource();
            tlasBuffers.mResult = gResourceManager->GetGPUResource(result)->As<GPUResourceDX12>()->GetResource();
            tlasBuffers.mInstanceDesc = gResourceManager->GetGPUResource(instance)->As<GPUResourceDX12>()->GetResource();

            topLevelASGenerator.Generate(ctx->GetDXRCommandList(), tlasBuffers.mScratch.Get(), tlasBuffers.mResult.Get(), tlasBuffers.mInstanceDesc.Get());
        };

        for (size_t i = 0; i < renderPassData.mRenderData.mSize; ++i)
        {
            const auto& rd = renderPassData.mRenderData[i];
            if (rd.mpMesh)
            {
                auto blas = createBlas(rd, (uint32_t)i);
                auto obj = Object{ blas, rd.mObjectData };
                blasList.push_back(obj);
                addToTlas(obj, (uint32_t)i, (uint32_t)0);
            }
        }

        if (renderPassData.mRenderData.mSize > 0)
        {
            createTlas();

            TextureDesc texDesc =
            {
                .mUsage = tex::USAGE_RT_ACCELERATION,
                .mBuffer = gResourceManager->GetGPUResourceHandle(ID("RTPass/TlasResult")),
                .mType = tex::TEXTURE_2D,
            };

            auto tlasUav = gResourceManager->CreateTexture(texDesc, ID("RTPass/TlasTex"));
            done = true;
        }
    }

    void RaytracePass::CreateOutputBuffer()
    {
        uint32_t height = gWindow->GetHeight();
        uint32_t width = gWindow->GetWidth();

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

        TextureDesc texDesc =
        { 
            .mUsage = tex::USAGE_UNORDERED,
            .mBuffer = outputBuffer, 
            .mType = tex::TEXTURE_2D,
        };

        auto outputBufferTexture = gResourceManager->CreateTexture(texDesc, ID("RTPass/OutputBufferTex"));
    }
    void RaytracePass::CreatePipeline()
    {
    }
    void RaytracePass::CreateShaderBindingTable()
    {
    }
#endif // NV_RENDERER_DX12
}

