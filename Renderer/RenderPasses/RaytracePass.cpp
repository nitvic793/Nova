#include "pch.h"
#include "RaytracePass.h"

#include <Renderer/CommonDefines.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Shader.h>
#include <Renderer/Window.h>
#include <Engine/Camera.h>
#include <Engine/EntityComponent.h>
#include <fmt/format.h>
#include <fmt/locale.h>
#include <Renderer/ConstantBufferPool.h>
#include <Debug/Error.h>

#if NV_RENDERER_DX12
#include <DX12/MeshDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <D3D12MemAlloc.h>
#include <DX12/DXR.h>
#include <DX12/ContextDX12.h>
#include <DX12/ShaderDX12.h>
#include <DX12/d3dx12.h>
#include <DX12/TextureDX12.h>

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
    using namespace asset;

#if NV_RENDERER_DX12

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

    struct RTObjects
    {
        Handle<GPUResource>         mOutputBuffer;
        ComPtr<ID3D12RootSignature> mGlobalRootSig;
        ComPtr<ID3D12RootSignature> mLocalRootSig;
        ComPtr<ID3D12StateObject>   mStateObject;
        ID3D12Resource*             mBlas;
        ID3D12Resource*             mTlas;
        ID3D12Resource*             mRayGenShaderTable;
        ID3D12Resource*             mMissShaderTable;
        ID3D12Resource*             mHitGroupShaderTable;

        RayGenConstantBuffer        mRayGenCB;
        TextureDX12*                mOutputUAV;
    };

    enum GlobalRS
    {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        CountGlobal
    };

    enum LocalRS
    {
        ViewportConstantSlot = 0,
        CountLocal
    };

    GPUResource* CreateUploadBuffer(size_t size, const char* name)
    {
        GPUResourceDesc desc = { .mSize = (uint32_t)size, .mInitialState = STATE_GENERIC_READ, .mBufferMode = BUFFER_MODE_UPLOAD };
        auto handle = gResourceManager->CreateResource(desc, ID(name));
        auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(handle);
        res->MapMemory();
        return res;
    };

    class ShaderRecord
    {
    public:
        ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize) :
            shaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
        {
        }

        ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize, void* pLocalRootArguments, UINT localRootArgumentsSize) :
            shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
            localRootArguments(pLocalRootArguments, localRootArgumentsSize)
        {
        }

        void CopyTo(void* dest) const
        {
            uint8_t* byteDest = static_cast<uint8_t*>(dest);
            memcpy(byteDest, shaderIdentifier.ptr, shaderIdentifier.size);
            if (localRootArguments.ptr)
            {
                memcpy(byteDest + shaderIdentifier.size, localRootArguments.ptr, localRootArguments.size);
            }
        }

        struct PointerWithSize {
            void* ptr;
            UINT size;

            PointerWithSize() : ptr(nullptr), size(0) {}
            PointerWithSize(void* _ptr, UINT _size) : ptr(_ptr), size(_size) {};
        };

        PointerWithSize shaderIdentifier;
        PointerWithSize localRootArguments;
    };

    inline UINT Align(UINT size, UINT alignment)
    {
        return (size + (alignment - 1)) & ~(alignment - 1);
    }

    class ShaderTable 
    {
        GPUResourceDX12* mpResource;
        uint8_t* m_mappedShaderRecords;
        UINT m_shaderRecordSize;

        // Debug support
        std::string m_name;
        std::vector<ShaderRecord> m_shaderRecords;

        ShaderTable() {}
    public:
        ShaderTable(ID3D12Device* device, UINT numShaderRecords, UINT shaderRecordSize, const char* resourceName = nullptr)
            : m_name(resourceName)
        {
            m_shaderRecordSize = Align(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
            m_shaderRecords.reserve(numShaderRecords);
            UINT bufferSize = numShaderRecords * m_shaderRecordSize;
            mpResource = (GPUResourceDX12*)CreateUploadBuffer(bufferSize, fmt::format("RT/ShaderTable_{}", resourceName).c_str());
            m_mappedShaderRecords = mpResource->GetMappedMemory();
        }

        ID3D12Resource* GetResource() const { return mpResource->GetResource().Get(); }

        void push_back(const ShaderRecord& shaderRecord)
        {
            assert(m_shaderRecords.size() < m_shaderRecords.capacity());
            m_shaderRecords.push_back(shaderRecord);
            shaderRecord.CopyTo(m_mappedShaderRecords);
            m_mappedShaderRecords += m_shaderRecordSize;
        }

        UINT GetShaderRecordSize() { return m_shaderRecordSize; }

        // Pretty-print the shader records.
        void DebugPrint(std::unordered_map<void*, std::wstring> shaderIdToStringMap)
        {
            std::wstringstream wstr;
            wstr << L"|--------------------------------------------------------------------\n";
            wstr << L"|Shader table - " << m_name.c_str() << L": "
                << m_shaderRecordSize << L" | "
                << m_shaderRecords.size() * m_shaderRecordSize << L" bytes\n";

            for (UINT i = 0; i < m_shaderRecords.size(); i++)
            {
                wstr << L"| [" << i << L"]: ";
                wstr << shaderIdToStringMap[m_shaderRecords[i].shaderIdentifier.ptr] << L", ";
                wstr << m_shaderRecords[i].shaderIdentifier.size << L" + " << m_shaderRecords[i].localRootArguments.size << L" bytes \n";
            }
            wstr << L"|--------------------------------------------------------------------\n";
            wstr << L"\n";
            OutputDebugStringW(wstr.str().c_str());
        }
    };

    void CheckRaytracingSupport(ID3D12Device* pDevice)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
            &options5, sizeof(options5));
        if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
            debug::ReportError("Raytracing not supported on this hardware");
    }

    void RaytracePass::Init()
    {
        mRtObjects = Alloc<RTObjects>();
        CreateOutputBuffer();
        CreatePipeline();
        auto device = (DeviceDX12*)gRenderer->GetDevice();
        CheckRaytracingSupport(device->GetDevice());

        mRtObjects->mRayGenCB.Viewport = { -1.0f, -1.0f, 1.0f, 1.0f };
        float border = 0.1f;
        uint32_t width = gWindow->GetWidth();
        uint32_t height = gWindow->GetHeight();
        float aspectRatio = (float)width / (float)height;

        mRtObjects->mRayGenCB.Stencil =
        {
            -1 + border / aspectRatio, -1 + border,
             1 - border / aspectRatio, 1.0f - border
        };
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
            cameraCbv = gpConstantBufferPool->GetConstantBuffer<FrameData>();
            return;
        }

        if (done)
        {
            uint32_t width = gWindow->GetWidth();
            uint32_t height = gWindow->GetHeight();
            auto ctx = (ContextDX12*)renderer->GetContext();
            const auto dispatchRays = [&](ID3D12StateObject* stateObject, D3D12_DISPATCH_RAYS_DESC* dispatchDesc)
            {
                auto commandList = ctx->GetDXRCommandList();
                dispatchDesc->HitGroupTable.StartAddress = mRtObjects->mHitGroupShaderTable->GetGPUVirtualAddress();
                dispatchDesc->HitGroupTable.SizeInBytes = mRtObjects->mHitGroupShaderTable->GetDesc().Width;
                dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
                dispatchDesc->MissShaderTable.StartAddress = mRtObjects->mMissShaderTable->GetGPUVirtualAddress();
                dispatchDesc->MissShaderTable.SizeInBytes = mRtObjects->mMissShaderTable->GetDesc().Width;
                dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
                dispatchDesc->RayGenerationShaderRecord.StartAddress = mRtObjects->mRayGenShaderTable->GetGPUVirtualAddress();
                dispatchDesc->RayGenerationShaderRecord.SizeInBytes = mRtObjects->mRayGenShaderTable->GetDesc().Width;
                dispatchDesc->Width = width;
                dispatchDesc->Height = height;
                dispatchDesc->Depth = 1;
                commandList->SetPipelineState1(stateObject);
                commandList->DispatchRays(dispatchDesc);
            };

            SetContextDefault(ctx);
            TransitionBarrier initBarriers[] = { {.mTo = STATE_UNORDERED_ACCESS, .mResource = mRtObjects->mOutputBuffer } };
            ctx->ResourceBarrier({ &initBarriers[0] , ArrayCountOf(initBarriers) });

            updateCameraParams();
            D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
            auto commandList = ctx->GetDXRCommandList();
            commandList->SetComputeRootSignature(mRtObjects->mGlobalRootSig.Get());
            commandList->SetComputeRootDescriptorTable(GlobalRS::OutputViewSlot, mRtObjects->mOutputUAV->GetGPUHandle());
            commandList->SetComputeRootShaderResourceView(GlobalRS::AccelerationStructureSlot, mRtObjects->mTlas->GetGPUVirtualAddress());
            dispatchRays(mRtObjects->mStateObject.Get(), &dispatchDesc);

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
        auto dxrDevice = device->GetDXRDevice();
        auto renderer = (RendererDX12*)gRenderer;
        auto ctx = (ContextDX12*)renderer->GetContext();

        const auto createUavBuffer = [](size_t size, State initState, const char* name)
        {
            GPUResourceDesc desc = { .mSize = (uint32_t)size, .mFlags = FLAG_ALLOW_UNORDERED, .mInitialState = initState };
            auto handle = gResourceManager->CreateResource(desc, ID(name));
            auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(handle);
            return res->GetResource().Get();
        };

        const auto createUploadBuffer = [](size_t size, void* pData, const char* name)
        {
            GPUResourceDesc desc = { .mSize = (uint32_t)size, .mInitialState = STATE_GENERIC_READ, .mBufferMode = BUFFER_MODE_UPLOAD };
            auto handle = gResourceManager->CreateResource(desc, ID(name));
            auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(handle);
            res->MapMemory();
            res->UploadMapped((uint8_t*)pData, size);
            res->UnmapMemory();
            return res->GetResource().Get();
        };

        if (renderPassData.mRenderData.mSize > 0)
        {
            auto mesh = (MeshDX12*)renderPassData.mRenderData.mppMeshes[1];
            auto objectData = renderPassData.mRenderData.mpObjectData[1];
            auto geomDesc = mesh->GetGeometryDescs();
            geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
            topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            topLevelInputs.Flags = buildFlags;
            topLevelInputs.NumDescs = 1;
            topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
            dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
            bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            bottomLevelInputs.pGeometryDescs = &geomDesc;
            dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);

            auto scratchResource = createUavBuffer(max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), STATE_UNORDERED_ACCESS, "RT/Scratch");
            mRtObjects->mBlas = createUavBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, STATE_RAYTRACING_STRUCTURE, "RT/Blas");
            mRtObjects->mTlas = createUavBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, STATE_RAYTRACING_STRUCTURE, "RT/Tlas");

            D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
            instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
            instanceDesc.InstanceMask = 1;
            instanceDesc.AccelerationStructure = mRtObjects->mBlas->GetGPUVirtualAddress();

            auto instanceDescs = createUploadBuffer(sizeof(instanceDesc), &instanceDesc, "RT/InstanceDescs");
            // Bottom Level Acceleration Structure desc
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
            {
                bottomLevelBuildDesc.Inputs = bottomLevelInputs;
                bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
                bottomLevelBuildDesc.DestAccelerationStructureData = mRtObjects->mBlas->GetGPUVirtualAddress();
            }

            // Top Level Acceleration Structure desc
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
            {
                topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
                topLevelBuildDesc.Inputs = topLevelInputs;
                topLevelBuildDesc.DestAccelerationStructureData = mRtObjects->mTlas->GetGPUVirtualAddress();
                topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
            }

            auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
            {
                raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
                CD3DX12_RESOURCE_BARRIER barriers[] = { CD3DX12_RESOURCE_BARRIER::UAV(mRtObjects->mBlas) };
                ctx->GetCommandList()->ResourceBarrier(_countof(barriers), &barriers[0]);
                raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
            };

            // Build acceleration structure.
            BuildAccelerationStructure(ctx->GetDXRCommandList());
            //createTlas();

            //TextureDesc texDesc =
            //{
            //    .mUsage = tex::USAGE_RT_ACCELERATION,
            //    .mBuffer = gResourceManager->GetGPUResourceHandle(ID("RTPass/TlasResult")),
            //    .mType = tex::TEXTURE_2D,
            //};

            //auto tlasUav = gResourceManager->CreateTexture(texDesc, ID("RTPass/TlasTex"));
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
            .mUseRayTracingHeap = false
        };

        auto outputBufferTexture = gResourceManager->CreateTexture(texDesc, ID("RTPass/OutputBufferTex"));
        mRtObjects->mOutputUAV = (TextureDX12*)gResourceManager->GetTexture(outputBufferTexture);
    }

    constexpr auto raygenShaderName = L"MyRaygenShader";
    constexpr auto closestHitShaderName = L"MyClosestHitShader";
    constexpr auto missShaderName = L"MyMissShader";
    constexpr auto hitGroupName = L"MyHitGroup";

    void RaytracePass::CreatePipeline()
    {
        auto renderer = (RendererDX12*)gRenderer;
        auto pDevice = (DeviceDX12*)renderer->GetDevice();
        auto dxrDevice = pDevice->GetDXRDevice();

        const auto serializeAndCreateRaytracingRootSignature = [&](D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
        {
            auto device = pDevice->GetDevice();
            ComPtr<ID3DBlob> blob;
            ComPtr<ID3DBlob> error;

            D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
            device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig)));
        };

        {
            CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
            UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
            CD3DX12_ROOT_PARAMETER rootParameters[GlobalRS::CountGlobal] = {};
            rootParameters[GlobalRS::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);
            rootParameters[GlobalRS::AccelerationStructureSlot].InitAsShaderResourceView(0);
            CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
            serializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &mRtObjects->mGlobalRootSig);
        }

        {
            CD3DX12_ROOT_PARAMETER rootParameters[LocalRS::CountLocal] = {};
            rootParameters[LocalRS::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(mRtObjects->mRayGenCB), 0, 0);
            CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
            localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
            serializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &mRtObjects->mLocalRootSig);
        }

        CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
        auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        auto shaderId = AssetID{ ASSET_SHADER, ID("Shaders/Raytracing.hlsl") };
        auto handle = gResourceManager->CreateShader({ shaderId, shader::LIB });
        auto shader = (ShaderDX12*)gResourceManager->GetShader(handle);

        D3D12_SHADER_BYTECODE libdxil = shader->GetByteCode();

        lib->SetDXILLibrary(&libdxil);
        {
            lib->DefineExport(raygenShaderName);
            lib->DefineExport(closestHitShaderName);
            lib->DefineExport(missShaderName);
        }
        
        auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        hitGroup->SetClosestHitShaderImport(closestHitShaderName);
        hitGroup->SetHitGroupExport(hitGroupName);
        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

        auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        UINT payloadSize = 4 * sizeof(float);   // float4 color
        UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
        shaderConfig->Config(payloadSize, attributeSize);

        // local Root Sig sub objects
        {
            auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            localRootSignature->SetRootSignature(mRtObjects->mLocalRootSig.Get());
            // Shader association
            auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
            rootSignatureAssociation->AddExport(raygenShaderName);
        }

        auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        globalRootSignature->SetRootSignature(mRtObjects->mGlobalRootSig.Get());

        auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        UINT maxRecursionDepth = 1; // ~ primary rays only. 
        pipelineConfig->Config(maxRecursionDepth);
        dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&mRtObjects->mStateObject));
    }

    void RaytracePass::CreateShaderBindingTable(const RenderPassData& renderPassData)
    {
        auto renderer = (RendererDX12*)gRenderer;
        auto pDevice = (DeviceDX12*)renderer->GetDevice();
        auto dxrDevice = pDevice->GetDXRDevice();

        void* rayGenShaderIdentifier;
        void* missShaderIdentifier;
        void* hitGroupShaderIdentifier;

        auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
        {
            rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(raygenShaderName);
            missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(missShaderName);
            hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(hitGroupName);
        };

        UINT shaderIdentifierSize;
        {
            ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
            mRtObjects->mStateObject.As(&stateObjectProperties);
            GetShaderIdentifiers(stateObjectProperties.Get());
            shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        }

        {
            struct RootArguments {
                RayGenConstantBuffer cb;
            } rootArguments;
            rootArguments.cb = mRtObjects->mRayGenCB;

            UINT numShaderRecords = 1;
            UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
            ShaderTable rayGenShaderTable(dxrDevice, numShaderRecords, shaderRecordSize, "RayGenShaderTable");
            rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
            mRtObjects->mRayGenShaderTable = rayGenShaderTable.GetResource();
        }

        // Miss shader table
        {
            UINT numShaderRecords = 1;
            UINT shaderRecordSize = shaderIdentifierSize;
            ShaderTable missShaderTable(dxrDevice, numShaderRecords, shaderRecordSize, "MissShaderTable");
            missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
            mRtObjects->mMissShaderTable = missShaderTable.GetResource();
        }

        // Hit group shader table
        {
            UINT numShaderRecords = 1;
            UINT shaderRecordSize = shaderIdentifierSize;
            ShaderTable hitGroupShaderTable(dxrDevice, numShaderRecords, shaderRecordSize, "HitGroupShaderTable");
            hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
            mRtObjects->mHitGroupShaderTable = hitGroupShaderTable.GetResource();
        }
    }

#endif // NV_RENDERER_DX12
}

