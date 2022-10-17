#include "pch.h"
#include "RaytracePass.h"
#include <DX12/NvidiaDXR/BottomLevelASGenerator.h>
#include <Renderer/CommonDefines.h>
#include <Renderer/ResourceManager.h>

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
    }

    void RaytracePass::Execute(const RenderPassData& renderPassData)
    {
    }

    void RaytracePass::Destroy()
    {
    }

    void RaytracePass::CreateRaytracingStructures(const RenderPassData& renderPassData)
    {
        auto device = (DeviceDX12*)gRenderer->GetDevice();
        auto renderer = (RendererDX12*)gRenderer;
        auto ctx = (ContextDX12*)renderer->GetContext();
        const auto createBlas = [&]()
        {
            AccelerationStructureBuffers buffers;
            BottomLevelASGenerator bottomLevelAS;
            for (size_t i = 0; i < renderPassData.mRenderData.mSize; ++i)
            {
                const auto& rd = renderPassData.mRenderData[i];
                if (rd.mpMesh)
                {
                    auto mesh = rd.mpMesh->As<MeshDX12>();
                    bottomLevelAS.AddVertexBuffer(
                        mesh->GetVertexBuffer(), 0, (uint32_t)mesh->GetVertexCount(), sizeof(Vertex),
                        mesh->GetIndexBuffer(), 0, (uint32_t)mesh->GetIndexCount(), nullptr, 0);
                }
            }

            UINT64 scratchSizeInBytes = 0;
            UINT64 resultSizeInBytes = 0;

            bottomLevelAS.ComputeASBufferSizes((ID3D12Device5*)device->GetDevice(), false, &scratchSizeInBytes,
                &resultSizeInBytes);

            GPUResourceDesc scratchDesc = { .mSize = (uint32_t)scratchSizeInBytes, .mFlags = FLAG_ALLOW_UNORDERED, .mInitialState = STATE_COMMON };
            GPUResourceDesc resultDesc = { .mSize = (uint32_t)resultSizeInBytes, .mFlags = FLAG_ALLOW_UNORDERED, .mInitialState = STATE_RAYTRACING_STRUCTURE };
            auto scratch = gResourceManager->CreateResource(scratchDesc, ID("RTPass/BlasScratch"));
            auto result = gResourceManager->CreateResource(resultDesc, ID("RTPass/BlasResult"));

            buffers.mScratch = gResourceManager->GetGPUResource(scratch)->As<GPUResourceDX12>()->GetResource();
            buffers.mResult = gResourceManager->GetGPUResource(result)->As<GPUResourceDX12>()->GetResource();

            bottomLevelAS.Generate(ctx->GetDXRCommandList(), buffers.mScratch.Get(), buffers.mResult.Get(), false, nullptr);
            return buffers;
        };

        const auto createTlas = [&]()
        {
            for (size_t i = 0; i < renderPassData.mRenderData.mSize; ++i)
            {
                const auto& rd = renderPassData.mRenderData[i];
                if (rd.mpMesh)
                {
                    auto transform = rd.mObjectData.World;
                }
            }
        };

        auto blas = createBlas();

    }
#endif
}

