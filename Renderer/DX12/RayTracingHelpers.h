#pragma once


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
	};

	void BuildAccelerationStructure(ID3D12GraphicsCommandList4* pCommandList, const std::vector<Mesh*>& meshes, RayTracingRuntimeData& outRtData);
}