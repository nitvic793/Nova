#include "pch.h"
#include "DeviceDX12.h"
#include <Renderer/Window.h>
#include <DX12/WindowDX12.h>
#include <Engine/Log.h>
#include <DX12/DirectXIncludes.h>

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

		IDXGIAdapter1* adapter;
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
			hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr);
			if (SUCCEEDED(hr))
			{
				adapterFound = true;
				break;
			}

			adapterIndex++;
		}

		if (!adapterFound)
		{
			log::Error("Feature level D3D_FEATURE_LEVEL_12_0 or greater required");
			return false;
		}

		hr = D3D12CreateDevice(
			adapter,
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(mDevice.ReleaseAndGetAddressOf())
		);

		adapter->Release();

		if (!SUCCEEDED(hr)) return false;

		// Disable error for copy descriptor 
		{
			ComPtr<ID3D12InfoQueue> d3dInfoQueue;
			if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
			{
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

				D3D12_MESSAGE_ID blockedIds[] = { D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				  D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE, D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES };
				D3D12_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.pIDList = blockedIds;
				filter.DenyList.NumIDs = 3;
				d3dInfoQueue->AddRetrievalFilterEntries(&filter);
				d3dInfoQueue->AddStorageFilterEntries(&filter);
			}
		}

		D3D12_COMMAND_QUEUE_DESC cqDesc = {};
		hr = mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf()));

		if (!SUCCEEDED(hr)) return false;

		if (!InitSwapChain(window, DXGI_FORMAT_R8G8B8A8_UNORM))
			return false;

		return true;
    }

	void DeviceDX12::Present()
	{
		mSwapChain->Present(1, 0);
	}

	DeviceDX12::~DeviceDX12()
	{
	}

	ID3D12Device* DeviceDX12::GetDevice() const
	{
		return mDevice.Get();
	}

	bool DeviceDX12::InitSwapChain(Window& window, DXGI_FORMAT format)
	{
		auto win = (WindowDX12*)&window;
		DXGI_MODE_DESC backBufferDesc = {};
		backBufferDesc.Width = window.GetWidth();
		backBufferDesc.Height = window.GetHeight();
		backBufferDesc.Format = format;
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1;

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = kFrameBufferCount;
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
			mSwapChain->ResizeBuffers(kFrameBufferCount, window.GetWidth(), window.GetHeight(), format, 0);
		}

		return true;
	}
}

