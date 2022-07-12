#include "pch.h"
#include "RendererDX12.h"
#include <DX12/DeviceDX12.h>
#include <DX12/DirectXIncludes.h>
#include <DX12/WindowDX12.h>
#include <Renderer/DescriptorHeap.h>
#include <DX12/DescriptorHeapDX12.h>

#include <dxgidebug.h>

namespace nv::graphics
{
    void RendererDX12::Init(Window& window)
    {
        constexpr uint32_t kDefaultDescriptorCount = 32;

        mDescriptorHeapPool.Init();
        mDevice = ScopedPtr<Device, true>((Device*)Alloc<DeviceDX12>());
        mDevice->Init(window);

        auto dxDevice = (DeviceDX12*)mDevice.Get();

        auto rtvDescriptorPool = (DescriptorHeapDX12*)mDescriptorHeapPool.CreateInstance(mRtvHeap);
        rtvDescriptorPool->Create(dxDevice->mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kDefaultDescriptorCount);

        auto dsvDescriptorPool = (DescriptorHeapDX12*)mDescriptorHeapPool.CreateInstance(mDsvHeap);
        dsvDescriptorPool->Create(dxDevice->mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kDefaultDescriptorCount);

        auto gpuDescriptorPool = (DescriptorHeapDX12*)mDescriptorHeapPool.CreateInstance(mGpuHeap);
        gpuDescriptorPool->Create(dxDevice->mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDefaultDescriptorCount);
    }

    void RendererDX12::Destroy()
    {
        mDescriptorHeapPool.Destroy();
    }

    void RendererDX12::Present()
    {
        auto device = (DeviceDX12*)mDevice.Get();
        device->Present();
    }

    RendererDX12::~RendererDX12()
    {
    }

    void ReportLeaksDX12()
    {
#ifdef _DEBUG
        IDXGIDebug1* pDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
        {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            pDebug->Release();
        }
#endif
    }
}

