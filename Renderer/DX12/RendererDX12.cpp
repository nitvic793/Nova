#include "pch.h"
#include "RendererDX12.h"
#include <Renderer/DescriptorHeap.h>
#include <DX12/DeviceDX12.h>
#include <DX12/DirectXIncludes.h>
#include <DX12/WindowDX12.h>
#include <DX12/DescriptorHeapDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/Interop.h>

#include <Debug/Error.h>

#include <dxgidebug.h>

namespace nv::graphics
{
    void RendererDX12::Init(Window& window)
    {
        constexpr uint32_t kDefaultDescriptorCount = 32;
        constexpr uint32_t kDefaultGPUDescriptorCount = 1024;

        mDescriptorHeapPool.Init();
        mDevice = ScopedPtr<Device, true>((Device*)Alloc<DeviceDX12>());
        mDevice->Init(window);

        auto dxDevice = (DeviceDX12*)mDevice.Get();

        auto initDescriptorHeap = [&](D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorCount, Handle<DescriptorHeap>& handle, bool shaderVisible = false)
        {
            auto descriptorHeap = (DescriptorHeapDX12*)mDescriptorHeapPool.CreateInstance(handle);
            descriptorHeap->Create(dxDevice->mDevice.Get(), type, descriptorCount, shaderVisible);
            return descriptorHeap;
        };

        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kDefaultDescriptorCount, mRtvHeap);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kDefaultDescriptorCount, mDsvHeap);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDefaultGPUDescriptorCount, mGpuHeap, true);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDefaultDescriptorCount, mTextureHeap);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDefaultDescriptorCount, mConstantBufferHeap);
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

    void RendererDX12::InitDependentResources()
    {
        assert(gResourceManager);

        const format::SurfaceFormat format = format::R8G8B8A8_UNORM;
        constexpr auto dxgiFormat = GetFormat(format);
        GPUResourceDX12* backBufferResources[FRAMEBUFFER_COUNT];

        auto dxDevice = (DeviceDX12*)mDevice.Get();
        auto rtvDescriptorHeap = mDescriptorHeapPool.GetAsDerived(mRtvHeap);

        for (int32_t i = 0; i < FRAMEBUFFER_COUNT; ++i)
            backBufferResources[i] = (GPUResourceDX12*)gResourceManager->Emplace(mpBackBuffers[i]);

        for (int32_t i = 0; i < FRAMEBUFFER_COUNT; ++i)
        {
            auto hr = dxDevice->mSwapChain->GetBuffer(
                i,
                IID_PPV_ARGS(backBufferResources[i]->GetResource().ReleaseAndGetAddressOf())
            );

            if (!SUCCEEDED(hr))
            {
                debug::ReportError("Unable to retrieve Swap Chain buffers");
            }
        }

        const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
        {
            .Format = dxgiFormat,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            .Texture2D = {.MipSlice = 0, .PlaneSlice = 0},
        };

        for (int32_t i = 0; i < FRAMEBUFFER_COUNT; ++i)
        {
            dxDevice->GetDevice()->CreateRenderTargetView(backBufferResources[i]->GetResource().Get(), &rtvDesc, rtvDescriptorHeap->HandleCPU(i));
            backBufferResources[i]->SetView(VIEW_RENDER_TARGET, DescriptorHandle(rtvDescriptorHeap->HandleCPU(i)));
        }
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

