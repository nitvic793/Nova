#include "pch.h"
#include "DeviceDX12.h"
#include <Engine/Log.h>
#include <Renderer/Window.h>
#include <DX12/WindowDX12.h>
#include <DX12/DirectXIncludes.h>
#include <DX12/Interop.h>
#include <D3D12MemAlloc.h>

#include <Debug/Profiler.h>

namespace nv::graphics
{
    bool DeviceDX12::Init(Window& window)
    {

#if NV_RENDERER_ENABLE_DEBUG_LAYER
		ID3D12Debug1* debugInterface;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
#endif
        auto hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(mDxgiFactory.ReleaseAndGetAddressOf()));

        if (!SUCCEEDED(hr)) return false;

		ComPtr<IDXGIAdapter1> adapter;
		UINT adapterIndex = 0;
		bool adapterFound = false;
		while (mDxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// we dont want a software device
				adapterIndex++;
				continue;
			}
			
			//direct3d 12 (feature level 12.1 or higher)
			hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr);
			if (SUCCEEDED(hr))
			{
				adapterFound = true;
				break;
			}

			adapterIndex++;
		}

		if (!adapterFound)
		{
			log::Error("Feature level D3D_FEATURE_LEVEL_12_1 or greater required");
			return false;
		}

		hr = D3D12CreateDevice(
			adapter.Get(),
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(mDevice.ReleaseAndGetAddressOf())
		);

		if (!SUCCEEDED(hr)) return false;

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = mDevice.Get();
		allocatorDesc.pAdapter = adapter.Get();

		hr = D3D12MA::CreateAllocator(&allocatorDesc, mGpuAllocator.ReleaseAndGetAddressOf());

		if (!SUCCEEDED(hr)) return false;

		// Disable error for copy descriptor 
		{
			ComPtr<ID3D12InfoQueue> d3dInfoQueue;
			if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
			{
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

				D3D12_MESSAGE_ID blockedIds[] =
				{
					// Workaround for descriptor copy error
					D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
					D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
					D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
					// Workarounds for debug layer issues on hybrid-graphics systems
					D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
					D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
				};

				D3D12_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.pIDList = blockedIds;
				filter.DenyList.NumIDs = _countof(blockedIds);
				d3dInfoQueue->AddRetrievalFilterEntries(&filter);
				d3dInfoQueue->AddStorageFilterEntries(&filter);
			}
		}

		D3D12_COMMAND_QUEUE_DESC cqDesc = {};
		hr = mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf()));

		if (!SUCCEEDED(hr)) return false;

		return true;
    }

	void DeviceDX12::Present()
	{
		constexpr bool vsyncEnabled = true;
		NV_GPU_FLIP(mSwapChain.Get());
		auto hr = mSwapChain->Present(vsyncEnabled ? 1 : 0, 0);
		if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			HRESULT reason = mDevice->GetDeviceRemovedReason();
#if defined(_DEBUG)
            wchar_t outString[100];
            size_t size = 100;
            swprintf_s(outString, size, L"Device removed! DXGI_ERROR code: 0x%X\n", reason);
            OutputDebugStringW(outString);
#endif
		}
	}

	DeviceDX12::~DeviceDX12()
	{
		BOOL fullscreen = false;
		mSwapChain->GetFullscreenState(&fullscreen, nullptr);
		if (fullscreen)
		{
			mSwapChain->SetFullscreenState(FALSE, nullptr);
		}
	}

	ID3D12Device* DeviceDX12::GetDevice() const
	{
		return mDevice.Get();
	}

	ID3D12Device9* DeviceDX12::GetDXRDevice() const
	{
		return mDXRDevice.Get();
	}

	D3D12MA::Allocator* DeviceDX12::GetAllocator() const
	{
		return mGpuAllocator.Get();
	}

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> DeviceDX12::GetCommandQueue() const
	{
		return mCommandQueue;
	}

	bool DeviceDX12::InitSwapChain(const Window& window, const format::SurfaceFormat format)
	{
		const auto dxgiFormat = GetFormat(format);
		auto win = (WindowDX12*)&window;
		DXGI_MODE_DESC backBufferDesc = {};
		backBufferDesc.Width = window.GetWidth();
		backBufferDesc.Height = window.GetHeight();
		backBufferDesc.Format = dxgiFormat;
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1;

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = FRAMEBUFFER_COUNT;
		swapChainDesc.BufferDesc = backBufferDesc;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 
		swapChainDesc.OutputWindow = win->GetWindowHandle();
		swapChainDesc.SampleDesc = sampleDesc;
		swapChainDesc.Windowed = !window.IsFullScreen();
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		auto hr = mDxgiFactory->CreateSwapChain(
			mCommandQueue.Get(),
			&swapChainDesc,
			(IDXGISwapChain**)mSwapChain.ReleaseAndGetAddressOf()
		);

		if (!SUCCEEDED(hr)) return false;

		if (window.IsFullScreen())
		{
			mSwapChain->SetFullscreenState(TRUE, nullptr);
			mSwapChain->ResizeBuffers(FRAMEBUFFER_COUNT, window.GetWidth(), window.GetHeight(), dxgiFormat, 0);
		}

		
		return true;
	}

	void DeviceDX12::InitRaytracingContext()
	{
		auto hr = mDevice->QueryInterface(IID_PPV_ARGS(mDXRDevice.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			log::Error("Unable to create ray tracing device context.");
		}
	}
}

