#include "pch.h"
#include "RayTracingHelpers.h"

#include <Renderer/Renderer.h>
#include <DX12/DeviceDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/ContextDX12.h>
#include <DX12/TextureDX12.h>

#include <d3dx12/d3dx12.h>
#include <D3D12MemAlloc.h>

#include <Debug/Profiler.h>

#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

namespace nv::graphics::dx12
{
	using namespace buffer;

	ID3D12Resource* CreateUavBuffer(size_t size, buffer::State initState, const char* name, Handle<GPUResource>* pOutHandle = nullptr)
	{
		GPUResourceDesc desc = { .mSize = (uint32_t)size, .mFlags = buffer::FLAG_ALLOW_UNORDERED, .mInitialState = initState, .mBufferMode = buffer::BUFFER_MODE_DEFAULT };
		auto handle = gResourceManager->CreateResource(desc, ID(name));
		auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(handle);
		if (pOutHandle) *pOutHandle = handle;
		return res->GetResource().Get();
	};

	ID3D12Resource* CreateUploadBuffer(size_t size, void* pData, const char* name, Handle<GPUResource>* pOutHandle = nullptr)
	{
		GPUResourceDesc desc = { .mSize = (uint32_t)size, .mInitialState = STATE_GENERIC_READ, .mBufferMode = BUFFER_MODE_UPLOAD };
		auto handle = gResourceManager->CreateResource(desc, ID(name));
		auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(handle);
		res->MapMemory();
		res->UploadMapped((uint8_t*)pData, size);
		res->UnmapMemory();
		if (pOutHandle) *pOutHandle = handle;
		return res->GetResource().Get();
	};

	inline void SetTransform(D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc, const float4x4& mat)
	{
        instanceDesc.Transform[0][0] = mat(0, 0);
        instanceDesc.Transform[0][1] = mat(0, 1);
        instanceDesc.Transform[0][2] = mat(0, 2);
        instanceDesc.Transform[0][3] = mat(0, 3);

        instanceDesc.Transform[1][0] = mat(1, 0);
        instanceDesc.Transform[1][1] = mat(1, 1);
        instanceDesc.Transform[1][2] = mat(1, 2);
        instanceDesc.Transform[1][3] = mat(1, 3);

        instanceDesc.Transform[2][0] = mat(2, 0);
        instanceDesc.Transform[2][1] = mat(2, 1);
        instanceDesc.Transform[2][2] = mat(2, 2);
        instanceDesc.Transform[2][3] = mat(2, 3);
	}

	void DestroyAccelerationStructure(RayTracingRuntimeData& rtData)
	{
		assert(rtData.mBLAS && rtData.mTLAS && rtData.mInstanceDescs);
        gRenderer->QueueDestroy(gResourceManager->GetGPUResourceHandle(ID("RT/Tlas")));
        gRenderer->QueueDestroy(gResourceManager->GetGPUResourceHandle(ID("RT/Blas")));
        gRenderer->QueueDestroy(gResourceManager->GetGPUResourceHandle(ID("RT/InstanceDescs")));
		gResourceManager->DestroyTexture(ID("RT/TlasSRV"));
    }

	void GetInstanceDescs(const std::vector<float4x4>& transforms, std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& outInstanceDescs, ID3D12Resource** pBlas)
	{
        outInstanceDescs.resize(transforms.size());
		for (uint32_t i = 0; i < transforms.size(); ++i)
		{
            D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = outInstanceDescs[i];
            memset(&instanceDesc, 0, sizeof(instanceDesc));
            SetTransform(instanceDesc, transforms[i]);
			instanceDesc.InstanceID = (UINT)i;
            instanceDesc.InstanceMask = 0xFF;
            instanceDesc.AccelerationStructure = pBlas[i]->GetGPUVirtualAddress();
        }
    }

	void UpdateAccelerationStructures(ID3D12GraphicsCommandList4* pCommandList,
		const std::vector<Mesh*>& meshes,
		const std::vector<float4x4>& transforms,
		RayTracingRuntimeData& outRtData)
	{
        NV_EVENT("RT/UpdateAccelerationStructure");

        std::vector< D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
        GetInstanceDescs(transforms, instanceDescs, outRtData.mBLASs.data());

		auto instancesResource = gResourceManager->GetGPUResource(gResourceManager->GetGPUResourceHandle(ID("RT/InstanceDescs")));
		instancesResource->MapMemory();
		instancesResource->UploadMapped((uint8_t*)instanceDescs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size());
		instancesResource->UnmapMemory();

        auto pRenderer = (RendererDX12*)gRenderer;
        auto pDevice = (DeviceDX12*)pRenderer->GetDevice();
        auto pDxrDevice = pDevice->GetDXRDevice();
		
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
        topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        topLevelInputs.NumDescs = (UINT)meshes.size();
        topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
        pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

		ID3D12Resource* pScratch = outRtData.mScratch;
		auto pPreviousTlas = outRtData.mTLAS;

        topLevelInputs.InstanceDescs = outRtData.mInstanceDescs->GetGPUVirtualAddress();
		topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc =
        {
            .DestAccelerationStructureData = outRtData.mTLAS->GetGPUVirtualAddress(),
            .Inputs = topLevelInputs,
			.SourceAccelerationStructureData = pPreviousTlas->GetGPUVirtualAddress(),
            .ScratchAccelerationStructureData = pScratch->GetGPUVirtualAddress(),
        };

        auto BuildAccelerationStructures = [&](ID3D12GraphicsCommandList4* raytracingCommandList)
            {
                raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
                CD3DX12_RESOURCE_BARRIER tlasBarriers[] = { CD3DX12_RESOURCE_BARRIER::UAV(outRtData.mTLAS) };
                raytracingCommandList->ResourceBarrier(_countof(tlasBarriers), &tlasBarriers[0]);
            };

        BuildAccelerationStructures(pCommandList);
	}

	void ClearResources(RayTracingRuntimeData& outRtData)
	{
		if (outRtData.mResourceList.empty())
			return;

		for (const auto handle : outRtData.mResourceList)
		{
			gResourceManager->QueueDestroy(handle, 16);
		}

		for (const auto handle : outRtData.mTextureList)
		{
			gResourceManager->DestroyTexture(handle);
		}

		outRtData.mTextureList.clear();
		outRtData.mResourceList.clear();
		outRtData.mBLASs.clear();
		outRtData.mBLAS = nullptr;
		outRtData.mInstanceDescs = nullptr;
		outRtData.mScratch = nullptr;
		outRtData.mTLAS = nullptr;
	}

	void BuildAccelerationStructure(ID3D12GraphicsCommandList4* pCommandList, 
		const std::vector<Mesh*>& meshes, 
		const std::vector<float4x4>& transforms, 
		RayTracingRuntimeData& outRtData, 
		bool bUpdateOnly)
	{
		NV_EVENT("RT/BuildAccelerationStructure");

		if (bUpdateOnly)
		{
			UpdateAccelerationStructures(pCommandList, meshes, transforms, outRtData);
			return;
		}

		ClearResources(outRtData);

		Handle<GPUResource> resourceHandle;
		const auto trackResource = [resList = &outRtData.mResourceList](Handle<GPUResource> handle)
		{
			resList->push_back(handle);
		};

		const auto trackTextures = [texList = &outRtData.mTextureList](Handle<Texture> handle)
		{
			texList->push_back(handle);
		};

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		for (auto& mesh : meshes)
		{
			MeshDX12* pMesh = (MeshDX12*)mesh;
			D3D12_RAYTRACING_GEOMETRY_DESC desc = pMesh->GetGeometryDescs();
			geometryDescs.push_back(desc);
		}

		auto pRenderer = (RendererDX12*)gRenderer;
		auto pDevice = (DeviceDX12*)pRenderer->GetDevice();
		auto pDxrDevice = pDevice->GetDXRDevice();


		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};

		std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> blasInputs;

		uint64_t maxBottomLevelBytes = 0;
		for (const auto& gDesc : geometryDescs)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
			bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			bottomLevelInputs.NumDescs = 1;
			bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			bottomLevelInputs.pGeometryDescs = &gDesc;
			blasInputs.push_back(bottomLevelInputs);
			pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
			if (bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > maxBottomLevelBytes)
				maxBottomLevelBytes = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
		}


		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
		topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		topLevelInputs.NumDescs = (UINT)meshes.size();
		topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
		pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

		const uint32_t scratchBufferSize = (uint32_t)ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, 
			std::max(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, maxBottomLevelBytes));
		
		ID3D12Resource* pScratch = CreateUavBuffer(scratchBufferSize, buffer::STATE_COMMON, "RT/Scratch", &resourceHandle);
		trackResource(resourceHandle);
		outRtData.mTLAS = CreateUavBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, STATE_RAYTRACING_STRUCTURE, "RT/Tlas", &resourceHandle);
		trackResource(resourceHandle);

		uint32_t idx = 0;
		for (const auto& blasInput : blasInputs)
		{
			pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&blasInput, &bottomLevelPrebuildInfo);
			std::string resname = "RT/Blas/";
			resname.append(std::to_string(idx));
			idx++;

			auto& resource = outRtData.mBLASs.emplace_back();
			resource = CreateUavBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, STATE_RAYTRACING_STRUCTURE, resname.c_str(), &resourceHandle);
			trackResource(resourceHandle);
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc =
			{
				.DestAccelerationStructureData = resource->GetGPUVirtualAddress(),
				.Inputs = blasInput,
				.ScratchAccelerationStructureData = pScratch->GetGPUVirtualAddress(),
			};

			pCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
			CD3DX12_RESOURCE_BARRIER blasBarriers[] = { CD3DX12_RESOURCE_BARRIER::UAV(resource) };
			pCommandList->ResourceBarrier(_countof(blasBarriers), &blasBarriers[0]);
		}


		std::vector< D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
		GetInstanceDescs(transforms, instanceDescs, outRtData.mBLASs.data());
		outRtData.mInstanceDescs = CreateUploadBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size(), instanceDescs.data(), "RT/InstanceDescs", &resourceHandle);
		trackResource(resourceHandle);
		topLevelInputs.InstanceDescs = outRtData.mInstanceDescs->GetGPUVirtualAddress();
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc =
		{
			.DestAccelerationStructureData = outRtData.mTLAS->GetGPUVirtualAddress(),
			.Inputs = topLevelInputs,
			.ScratchAccelerationStructureData = pScratch->GetGPUVirtualAddress(),
		};

		auto BuildAccelerationStructures = [&](ID3D12GraphicsCommandList4* raytracingCommandList)
		{
			raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
			CD3DX12_RESOURCE_BARRIER tlasBarriers[] = { CD3DX12_RESOURCE_BARRIER::UAV(outRtData.mTLAS) };
			raytracingCommandList->ResourceBarrier(_countof(tlasBarriers), &tlasBarriers[0]);
		};

		BuildAccelerationStructures(pCommandList);

		TextureDesc desc = { .mUsage = tex::USAGE_RT_ACCELERATION, .mBuffer = gResourceManager->GetGPUResourceHandle(ID("RT/Tlas")) };
		auto texHandle = gResourceManager->CreateTexture(desc, ID("RT/TlasSRV"));
		trackTextures(texHandle);
		outRtData.mScratch = pScratch;
	}
}
