#include "pch.h"
#include "RayTracingHelpers.h"

#include <Renderer/Renderer.h>
#include <DX12/DeviceDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/ContextDX12.h>
#include <DX12/TextureDX12.h>

#include <DX12/d3dx12.h>
#include <D3D12MemAlloc.h>

#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

namespace nv::graphics::dx12
{
	using namespace buffer;

	ID3D12Resource* CreateUavBuffer(size_t size, buffer::State initState, const char* name)
	{
		GPUResourceDesc desc = { .mSize = (uint32_t)size, .mFlags = buffer::FLAG_ALLOW_UNORDERED, .mInitialState = initState, .mBufferMode = buffer::BUFFER_MODE_DEFAULT };
		auto handle = gResourceManager->CreateResource(desc, ID(name));
		auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(handle);
		return res->GetResource().Get();
	};

	ID3D12Resource* CreateUploadBuffer(size_t size, void* pData, const char* name)
	{
		GPUResourceDesc desc = { .mSize = (uint32_t)size, .mInitialState = STATE_GENERIC_READ, .mBufferMode = BUFFER_MODE_UPLOAD };
		auto handle = gResourceManager->CreateResource(desc, ID(name));
		auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(handle);
		res->MapMemory();
		res->UploadMapped((uint8_t*)pData, size);
		res->UnmapMemory();
		return res->GetResource().Get();
	};

	void BuildAccelerationStructure(ID3D12GraphicsCommandList4* pCommandList, const std::vector<Mesh*>& meshes, RayTracingRuntimeData& outRtData)
	{
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		for (auto& mesh : meshes)
		{
			MeshDX12* pMesh = (MeshDX12*)mesh;
			const D3D12_RAYTRACING_GEOMETRY_DESC desc = pMesh->GetGeometryDescs();
			geometryDescs.push_back(desc);
		}

		auto pRenderer = (RendererDX12*)gRenderer;
		auto pDevice = (DeviceDX12*)pRenderer->GetDevice();
		auto pDxrDevice = pDevice->GetDXRDevice();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
		topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		topLevelInputs.NumDescs = 1;
		topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
		pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
		bottomLevelInputs.NumDescs = (UINT)geometryDescs.size();
		bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomLevelInputs.pGeometryDescs = geometryDescs.data();
		pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);

		const uint32_t scratchBufferSize = (uint32_t)ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, std::max(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes));
		
		ID3D12Resource* pScratch = CreateUavBuffer(scratchBufferSize, buffer::STATE_COMMON, "RT/Scratch");
		outRtData.mBLAS = CreateUavBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, STATE_RAYTRACING_STRUCTURE, "RT/Blas");
		outRtData.mTLAS = CreateUavBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, STATE_RAYTRACING_STRUCTURE, "RT/Tlas");

		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
		memset(&instanceDesc, 0, sizeof(instanceDesc));
		instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
		instanceDesc.InstanceMask = 1;
		instanceDesc.AccelerationStructure = outRtData.mBLAS->GetGPUVirtualAddress();

		outRtData.mInstanceDescs = CreateUploadBuffer(sizeof(instanceDesc), &instanceDesc, "RT/InstanceDescs");

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc =
		{
			.DestAccelerationStructureData = outRtData.mBLAS->GetGPUVirtualAddress(),
			.Inputs = bottomLevelInputs,
			.ScratchAccelerationStructureData = pScratch->GetGPUVirtualAddress(),
		};

		topLevelInputs.InstanceDescs = outRtData.mInstanceDescs->GetGPUVirtualAddress();
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc =
		{
			.DestAccelerationStructureData = outRtData.mTLAS->GetGPUVirtualAddress(),
			.Inputs = topLevelInputs,
			.ScratchAccelerationStructureData = pScratch->GetGPUVirtualAddress(),
		};

		auto BuildAccelerationStructures = [&](ID3D12GraphicsCommandList4* raytracingCommandList)
		{
			raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
			CD3DX12_RESOURCE_BARRIER blasBarriers[] = { CD3DX12_RESOURCE_BARRIER::UAV(outRtData.mBLAS) };
			raytracingCommandList->ResourceBarrier(_countof(blasBarriers), &blasBarriers[0]);

			raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
			CD3DX12_RESOURCE_BARRIER tlasBarriers[] = { CD3DX12_RESOURCE_BARRIER::UAV(outRtData.mTLAS) };
			raytracingCommandList->ResourceBarrier(_countof(tlasBarriers), &tlasBarriers[0]);
		};

		BuildAccelerationStructures(pCommandList);

		TextureDesc desc = { .mUsage = tex::USAGE_RT_ACCELERATION, .mBuffer = gResourceManager->GetGPUResourceHandle(ID("RT/Tlas")) };
		gResourceManager->CreateTexture(desc, ID("RT/TlasSRV"));

		gRenderer->QueueDestroy(gResourceManager->GetGPUResourceHandle(ID("RT/Scratch")));
	}
}
