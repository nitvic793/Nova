#include "pch.h"
#include "RaytracePass.h"

#include <Renderer/CommonDefines.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Shader.h>
#include <Renderer/Window.h>
#include <Engine/Camera.h>
#include <Engine/EntityComponent.h>
#include <fmt/format.h>

#if NV_RENDERER_DX12
#include <DX12/MeshDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <D3D12MemAlloc.h>
#include <DX12/DXR.h>
#include <DX12/ContextDX12.h>
#include <DX12/ShaderDX12.h>

// Nvidia DXR Helpers
#include <DX12/NvidiaDXR/BottomLevelASGenerator.h>
#include <DX12/NvidiaDXR/TopLevelASGenerator.h>
#include <DX12/NvidiaDXR/RaytracingPipelineGenerator.h>
#include <DX12/NvidiaDXR/RootSignatureGenerator.h>
#include <DX12/NvidiaDXR/ShaderBindingTableGenerator.h>
#endif

namespace nv::graphics
{
    using namespace nv_helpers_dx12;
    using namespace buffer;

#if NV_RENDERER_DX12

    struct Object
    {
        AccelerationStructureBuffers    mBlas;
        ObjectData                      mData;
    };

    struct RTObjects
    {
        ComPtr<ID3D12RootSignature>         mRayGenSig;
        ComPtr<ID3D12RootSignature>         mMissShaderSig;
        ComPtr<ID3D12RootSignature>         mHitShaderSig;
        ComPtr<ID3D12StateObject>           mRtStateObject;
        ComPtr<ID3D12StateObjectProperties> mRtStateobjectProperties;

        ShaderBindingTableGenerator         mSbtHelper;

        Handle<GPUResource>                 mOutputBuffer;
        std::vector<Object>                 mBlasList;
    };

    void CheckRaytracingSupport(ID3D12Device* pDevice)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
            &options5, sizeof(options5));
        if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
            throw std::runtime_error("Raytracing not supported on device");
    }

    void RaytracePass::Init()
    {
        mRtObjects = Alloc<RTObjects>();
        CreateOutputBuffer();
        CreatePipeline();
        auto device = (DeviceDX12*)gRenderer->GetDevice();
        CheckRaytracingSupport(device->GetDevice());
    }

    bool done = false;
    ConstantBufferView cameraCbv;
    void RaytracePass::Execute(const RenderPassData& renderPassData)
    {
        auto renderer = (RendererDX12*)gRenderer;
        const auto updateCameraParams = [&]()
        {
            auto camera = ecs::gComponentManager.GetComponents<CameraComponent>()[0].mCamera;
            auto view = camera.GetViewTransposed();
            auto proj = camera.GetProjTransposed();
            auto projI = camera.GetProjInverseTransposed();
            auto viewI = camera.GetViewInverseTransposed();
            auto dirLights = ecs::gComponentManager.GetComponents<DirectionalLight>();

            FrameData data = { .View = view, .Projection = proj, .ViewInverse = viewI, .ProjectionInverse = projI };
            if (dirLights.Size() > 0)
            {
                for (uint32_t i = 0; i < dirLights.Size() && i < MAX_DIRECTIONAL_LIGHTS; ++i)
                {
                    data.DirLights[i] = dirLights[i];
                }

                data.DirLightsCount = min((uint32_t)MAX_DIRECTIONAL_LIGHTS, (uint32_t)dirLights.Size());
            }

            renderer->UploadToConstantBuffer(cameraCbv, (uint8_t*)&data, sizeof(FrameData));
        };

        if (!done && renderPassData.mRenderData.mSize > 0)
        {
            CreateRaytracingStructures(renderPassData);
            CreateShaderBindingTable(renderPassData);
            // FIXME: Causes flickering. Raytracing heap not working as expected. 
            //cameraCbv =  renderer->CreateConstantBuffer({ .mSize = sizeof(FrameData), .mUseRayTracingHeap = true }); 
        }

        if (done)
        {
            auto ctx = (ContextDX12*)renderer->GetContext();
            const auto dispatchRays = [&]()
            {
                auto& sbtHelper = mRtObjects->mSbtHelper;
                auto commandList = ctx->GetDXRCommandList();
                auto handle = gResourceManager->GetGPUResourceHandle(ID("RT/ShaderBindingTable"));
                auto sbtStorage = ((GPUResourceDX12*)gResourceManager->GetGPUResource(handle))->GetResource().Get();;

                D3D12_DISPATCH_RAYS_DESC desc = {};
                const uint32_t rayGenerationSectionSizeInBytes =
                    sbtHelper.GetRayGenSectionSize();
                desc.RayGenerationShaderRecord.StartAddress =
                    sbtStorage->GetGPUVirtualAddress();
                desc.RayGenerationShaderRecord.SizeInBytes =
                    rayGenerationSectionSizeInBytes;

                const uint32_t missSectionSizeInBytes = sbtHelper.GetMissSectionSize();
                desc.MissShaderTable.StartAddress =
                    sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
                desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
                desc.MissShaderTable.StrideInBytes = sbtHelper.GetMissEntrySize();

                const uint32_t hitGroupsSectionSize = sbtHelper.GetHitGroupSectionSize();
                desc.HitGroupTable.StartAddress = sbtStorage->GetGPUVirtualAddress() +
                    rayGenerationSectionSizeInBytes +
                    missSectionSizeInBytes;
                desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
                desc.HitGroupTable.StrideInBytes = sbtHelper.GetHitGroupEntrySize();

                desc.Width = gWindow->GetWidth();
                desc.Height = gWindow->GetHeight();
                desc.Depth = 1;

                commandList->SetPipelineState1(mRtObjects->mRtStateObject.Get());
                //commandList->DispatchRays(&desc);
            };

            SetContextDefault(ctx);
            TransitionBarrier initBarriers[] = { {.mTo = STATE_UNORDERED_ACCESS, .mResource = mRtObjects->mOutputBuffer } };
            ctx->ResourceBarrier({ &initBarriers[0] , ArrayCountOf(initBarriers) });

            //updateCameraParams();
            dispatchRays();

            TransitionBarrier endBarriers[] = { {.mTo = STATE_COPY_SOURCE, .mResource = mRtObjects->mOutputBuffer } };
            ctx->ResourceBarrier({ &endBarriers[0] , ArrayCountOf(endBarriers) });
        }
    }

    void RaytracePass::Destroy()
    {
        Free(mRtObjects);
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
                mRtObjects->mBlasList.push_back(obj);
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
        mRtObjects->mOutputBuffer = outputBuffer;
        TextureDesc texDesc =
        { 
            .mUsage = tex::USAGE_UNORDERED,
            .mBuffer = outputBuffer, 
            .mType = tex::TEXTURE_2D,
            .mUseRayTracingHeap = true
        };

        auto outputBufferTexture = gResourceManager->CreateTexture(texDesc, ID("RTPass/OutputBufferTex"));

        //TODO: Create camera frame data in RT Heap
    }

    void CreateLocalRootSignatures(ID3D12Device5* pDevice, ID3D12RootSignature** ppRayGenSig, ID3D12RootSignature** ppMissSig, ID3D12RootSignature** ppHitSig)
    {
        RootSignatureGenerator rsc;
        rsc.AddHeapRangesParameter(
            {
                {
                    0 /*u0*/,
                    1 /*1 descriptor */,
                    0 /*use the implicit register space 0*/,
                    D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/,
                    0 /*heap slot where the UAV is defined*/
                },
                {
                    0 /*t0*/,
                    1,
                    0,
                    D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
                    1
                },
                {
                    0 /*b0*/,
                    1,
                    0,
                    D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Camera parameters*/,
                    2
                }
            });

        *ppRayGenSig = rsc.Generate(pDevice, true);

        RootSignatureGenerator missRsc;
        *ppMissSig = missRsc.Generate(pDevice, true);


        RootSignatureGenerator hitRsc;
        hitRsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0);
        *ppHitSig = hitRsc.Generate(pDevice, true);
    }

    void RaytracePass::CreatePipeline()
    {
        auto device = (DeviceDX12*)gRenderer->GetDevice();

        ID3D12Device5* pDevice = (ID3D12Device5*)device->GetDevice();
        RayTracingPipelineGenerator pipeline(pDevice);

        auto rgId = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/RT/RayGen.hlsl") };
        auto missId = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/RT/Miss.hlsl") };
        auto hitId = asset::AssetID{ asset::ASSET_SHADER, ID("Shaders/RT/Hit.hlsl") };

        auto rayGenShaderHandle = gResourceManager->CreateShader({ rgId, shader::LIB });
        auto missShaderHandle = gResourceManager->CreateShader({ missId, shader::LIB });
        auto hitShaderHandle = gResourceManager->CreateShader({ hitId, shader::LIB });

        auto rayGenShader = (ShaderDX12*)gResourceManager->GetShader(rayGenShaderHandle);
        auto missShader = (ShaderDX12*)gResourceManager->GetShader(missShaderHandle);
        auto hitShader = (ShaderDX12*)gResourceManager->GetShader(hitShaderHandle);

        pipeline.AddLibrary(rayGenShader->GetBlob(), { L"RayGen" });
        pipeline.AddLibrary(missShader->GetBlob(), { L"Miss" });
        pipeline.AddLibrary(hitShader->GetBlob(), { L"ClosestHit", L"PlaneClosestHit" });
        pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
        pipeline.AddHitGroup(L"PlaneHitGroup", L"PlaneClosestHit");

        CreateLocalRootSignatures(pDevice, mRtObjects->mRayGenSig.GetAddressOf(), mRtObjects->mMissShaderSig.GetAddressOf(), mRtObjects->mHitShaderSig.GetAddressOf());
        pipeline.AddRootSignatureAssociation(mRtObjects->mRayGenSig.Get(), { L"RayGen" });
        pipeline.AddRootSignatureAssociation(mRtObjects->mMissShaderSig.Get(), { L"Miss" });
        pipeline.AddRootSignatureAssociation(mRtObjects->mHitShaderSig.Get(), { L"HitGroup", L"PlaneHitGroup" });
        pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + Distance
        pipeline.SetMaxAttributeSize(2 * sizeof(float));
        pipeline.SetMaxRecursionDepth(1);

        pipeline.Generate(mRtObjects->mRtStateObject.GetAddressOf());
        mRtObjects->mRtStateObject->QueryInterface(IID_PPV_ARGS(&mRtObjects->mRtStateobjectProperties));
    }

    void RaytracePass::CreateShaderBindingTable(const RenderPassData& renderPassData)
    {
        auto& sbtHelper = mRtObjects->mSbtHelper;
        sbtHelper.Reset();

        auto renderer = (RendererDX12*)gRenderer;
        auto startHandle = renderer->GetRTDescriptorHandle(0);
        uint64_t* heapPtr = reinterpret_cast<uint64_t*>(startHandle.ptr);

        sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPtr });
        sbtHelper.AddMissProgram(L"Miss", {});

        auto objDescriptors = renderPassData.mRenderDataArray.GetObjectDescriptors();
        for (uint32_t i = 0; i < objDescriptors.Size(); ++i)
        {
            auto vAddress = renderer->GetConstBufferAddress(objDescriptors[i]);
            sbtHelper.AddHitGroup(
                L"HitGroup",
                { reinterpret_cast<void*>(vAddress) });
        }

        sbtHelper.AddHitGroup(L"PlaneHitGroup", {});
        const uint32_t sbtSize = sbtHelper.ComputeSBTSize();

        GPUResourceDesc sbtStorageDesc = { .mSize = (uint32_t)sbtSize,.mInitialState = STATE_GENERIC_READ, .mBufferMode = BUFFER_MODE_UPLOAD };
        auto sbtStorageResourceHandle = gResourceManager->CreateResource(sbtStorageDesc, ID("RT/ShaderBindingTable"));
        auto sbtResource = (GPUResourceDX12*)gResourceManager->GetGPUResource(sbtStorageResourceHandle);
        sbtHelper.Generate(sbtResource->GetResource().Get(), mRtObjects->mRtStateobjectProperties.Get());
    }

#endif // NV_RENDERER_DX12
}

