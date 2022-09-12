#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DX12/GPUResourceDX12.h>
#include <DX12/ResourceManagerDX12.h>

namespace nv::graphics
{
	constexpr uint64_t CONST_BUFFER_ALIGNMENT_SIZE = 256;

	class GPUConstantBuffer
	{
	public:
		void Initialize(const uint64_t cbSize, const uint32_t count)
		{
			mAlignedCBSize = (cbSize + CONST_BUFFER_ALIGNMENT_SIZE - 1) & ~(CONST_BUFFER_ALIGNMENT_SIZE - 1);
			mBufferSize = mAlignedCBSize * count;
			CreateResource((uint32_t)mBufferSize);
			auto resource = GetResource();
			resource->MapMemory();
		}

		void Initialize(const uint32_t bufferSize)
		{
			mAlignedCBSize = CONST_BUFFER_ALIGNMENT_SIZE;
			mBufferSize = bufferSize;
			CreateResource((uint32_t)mBufferSize);
			auto resource = GetResource();
			resource->MapMemory();
		}

		void CopyData(void* data, uint64_t size, uint32_t index) const
		{
			uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
			GetResource()->UploadMapped(ptr, size, mAlignedCBSize * index);
		}

		void CopyDataOffset(void* data, uint64_t size, uint64_t offset) const
		{
			uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
			GetResource()->UploadMapped(ptr, size, offset);
		}

		void CopyData(void* data, uint64_t size) const
		{
			uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
			GetResource()->UploadMapped(ptr, size);
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetAddress(uint32_t index = 0) const
		{
			auto resource = (GPUResourceDX12*)gResourceManager->GetGPUResource(mResourceHandle);
			return resource->GetResource()->GetGPUVirtualAddress() + (size_t)mAlignedCBSize * index;
		}

		~GPUConstantBuffer() {}

	private:
		void CreateResource(uint32_t size)
		{
			GPUResourceDesc desc = GPUResourceDesc::UploadConstBuffer(size);
			mResourceHandle = gResourceManager->CreateResource(desc);
		}

		inline GPUResource* GetResource() const
		{
			return gResourceManager->GetGPUResource(mResourceHandle);
		}

	private:
		Handle<GPUResource> mResourceHandle = Null<GPUResource>();
		uint64_t			mAlignedCBSize	= 0;
		uint64_t			mBufferSize		= 0;
		char*				mAddressBegin	= nullptr;
	};
}

