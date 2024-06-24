#pragma once

#include <Renderer/DescriptorHeap.h>
#include <Lib/Assert.h>
#include <cstdint>
#include <wrl/client.h>
#include <d3d12.h>
#include <Lib/Vector.h>

namespace nv::graphics
{
	class DescriptorHeapDX12 : public DescriptorHeap
	{
	public:
		HRESULT Create(
			ID3D12Device* pDevice,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT NumDescriptors,
			bool bShaderVisible = false)
		{
			mHeapDesc.Type = Type;
			mHeapDesc.NumDescriptors = NumDescriptors;
			mHeapDesc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : (D3D12_DESCRIPTOR_HEAP_FLAGS)0);

			HRESULT hr = pDevice->CreateDescriptorHeap(&mHeapDesc,IID_PPV_ARGS(mpDescriptorHeap.ReleaseAndGetAddressOf()));

			if (FAILED(hr)) return hr;

			mpDescriptorHeap->SetName(L"Descriptor Heap");
			mCPUHeapStart = mpDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			if (bShaderVisible)
			{
				mGPUHeapStart = mpDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			}
			else
			{
				mGPUHeapStart.ptr = 0;
			}

			mHandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(mHeapDesc.Type);
			return hr;
		}

		operator ID3D12DescriptorHeap* () const { return mpDescriptorHeap.Get(); }
		ID3D12DescriptorHeap* Get() const { return mpDescriptorHeap.Get(); }

		UINT64 MakeOffsetted(UINT64 ptr, UINT index) const
		{
			UINT64 offsetted;
			offsetted = ptr + static_cast<UINT64>(index * mHandleIncrementSize);
			return offsetted;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE HandleCPU(UINT index) const
		{
			D3D12_CPU_DESCRIPTOR_HANDLE handle;
			handle.ptr = MakeOffsetted(mCPUHeapStart.ptr, index);
			return handle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE HandleGPU(UINT index) const
		{
			assert(mHeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
			D3D12_GPU_DESCRIPTOR_HANDLE handle;
			handle.ptr = MakeOffsetted(mGPUHeapStart.ptr, index);
			return handle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetCurrentGPUHandle() const
		{
			return HandleGPU(GetCurrentIndex());
		}

		uint32_t GetCurrentIndex() const 
		{ 
			assert(mSize > 0); 
			return mSize - 1;  
		}

		constexpr uint32_t GetSize() const { return mSize; }

		D3D12_CPU_DESCRIPTOR_HANDLE PushCPU(uint32_t* outIndex = nullptr)
		{
			uint32_t index;
			D3D12_CPU_DESCRIPTOR_HANDLE result;

			if (mFreeIndices.IsEmpty())
			{
				index = mSize;
				result = HandleCPU(index);
				mSize++;
			}
			else
			{
				index = mFreeIndices.Pop();
				result = HandleCPU(index);
			}

			if (outIndex)
				*outIndex = index;

			return result;
		}

#pragma optimize ("", off)
		D3D12_GPU_DESCRIPTOR_HANDLE PushGPU()
		{
			if (mFreeIndices.IsEmpty())
				return HandleGPU(mSize++);
			else
				return HandleGPU(mFreeIndices.Pop());
		}
#pragma optimize ("", on)

		void Remove(uint32_t index)
		{
			mFreeIndices.Push(index);
		}

		constexpr bool IsShaderVisible() const
		{
			return mHeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		}

		constexpr D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const
		{
			return mHeapDesc.Type;
		}

		~DescriptorHeapDX12() { }

	private:
		D3D12_DESCRIPTOR_HEAP_DESC						mHeapDesc;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	mpDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE						mCPUHeapStart;
		D3D12_GPU_DESCRIPTOR_HANDLE						mGPUHeapStart;
		UINT											mHandleIncrementSize;
		uint32_t										mSize = 0;
		nv::Vector<uint32_t>							mFreeIndices;
	};
}