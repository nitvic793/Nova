#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DX12/ResourceManagerDX12.h>

namespace nv::graphics
{
	constexpr uint64_t ConstBufferAlignmentSize = 256;

	class GPUConstantBuffer
	{
	public:
		GPUConstantBuffer() {};
		void Initialize(ResourceManager* rm, const uint64_t cbSize, const uint32_t count)
		{
			constBufferSize = (cbSize + ConstBufferAlignmentSize - 1) & ~(ConstBufferAlignmentSize - 1);
			bufferSize = constBufferSize * count;
			//mResourceHandle = rm->CreateResource(
			//	CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			//	nullptr,
			//	D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_FLAG_NONE,
			//	CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
			//);

			//resource = rm->GetResource(mResourceHandle);
			CD3DX12_RANGE readRange(0, 0);
			resource->Map(0, &readRange, reinterpret_cast<void**>(&vAddressBegin));
		}

		void Initialize(ResourceManager* rm, const uint64_t bufferSize)
		{
			constBufferSize = ConstBufferAlignmentSize;
			this->bufferSize = bufferSize;
			//mResourceHandle = rm->CreateResource(
			//	CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			//	nullptr,
			//	D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_FLAG_NONE,
			//	CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
			//);

			//resource = rm->GetResource(mResourceHandle);
			CD3DX12_RANGE readRange(0, 0);
			resource->Map(0, &readRange, reinterpret_cast<void**>(&vAddressBegin));
		}

		void CopyData(void* data, uint64_t size, uint32_t index) const
		{
			char* ptr = reinterpret_cast<char*>(vAddressBegin) + (size_t)constBufferSize * index;
			memcpy(ptr, data, size);
		}

		void CopyData(void* data, uint64_t size, uint64_t offset) const
		{
			char* ptr = reinterpret_cast<char*>(vAddressBegin) + offset;
			memcpy(ptr, data, size);
		}

		void CopyData(void* data, uint64_t size) const
		{
			memcpy(vAddressBegin, data, size);
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetAddress(uint32_t index = 0) const
		{
			return resource->GetGPUVirtualAddress() + (size_t)constBufferSize * index;
		}

		~GPUConstantBuffer() {}

	private:
		ID3D12Resource* resource;
		Handle<GPUResource> mResourceHandle;
		uint64_t constBufferSize;
		uint64_t bufferSize;
		char* vAddressBegin;
	};
}

