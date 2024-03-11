#pragma once

#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#include <Renderer/DescriptorHeap.h>
#include <Lib/Assert.h>
#include <cstdint>
#include <wrl/client.h>
#include <d3d12.h>
#include <Lib/Vector.h>
#include <Renderer/Mesh.h>
#include <DX12/MeshDX12.h>

namespace nv::graphics::dx12
{
	struct RayTracingRuntimeData
	{
		ID3D12Resource* mTLAS			= nullptr;
		ID3D12Resource* mBLAS			= nullptr;
        ID3D12Resource* mInstanceDescs	= nullptr;
        ID3D12Resource* mScratch		= nullptr;

		std::vector<ID3D12Resource*> mBLASs;
	};

	void BuildAccelerationStructure(ID3D12GraphicsCommandList4* pCommandList, 
		const std::vector<Mesh*>& meshes, 
		const std::vector<float4x4>& transforms, 
		RayTracingRuntimeData& outRtData, 
		bool bUpdateOnly = false);
}