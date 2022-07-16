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

    void RendererDX12::InitFrameBuffers(const Window& window, const format::SurfaceFormat format)
    {
        assert(gResourceManager);
        const auto dxgiFormat = GetFormat(format);
        if (!mDevice->InitSwapChain(window, format))
            return;

        GPUResourceDX12* backBufferResources[FRAMEBUFFER_COUNT];

        auto dxDevice = (DeviceDX12*)mDevice.Get();
        auto rtvDescriptorHeap = mDescriptorHeapPool.GetAsDerived(mRtvHeap);
        auto dsvDescriptorHeap = mDescriptorHeapPool.GetAsDerived(mDsvHeap);

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

        const auto dsvFormat = format::D32_FLOAT;
        
        auto depthHandle = gResourceManager->CreateResource(
            {
                .mWidth = gWindow->GetWidth(),
                .mHeight = gWindow->GetHeight(),
                .mFormat = dsvFormat,
                .mType = buffer::TYPE_TEXTURE_2D,
                .mFlags = buffer::FLAG_ALLOW_DEPTH,
                .mInitialState = buffer::STATE_DEPTH_WRITE,
                .mClearValue = {.mFormat = dsvFormat,.mColor = {1.f,0,0,0} , .mStencil = 0}
            }
        );

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Texture2D.MipSlice = 0;
        dsvDesc.Format = GetFormat(dsvFormat);
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        auto depthResource = (GPUResourceDX12*)gResourceManager->GetGPUResource(depthHandle);
        dxDevice->mDevice->CreateDepthStencilView(depthResource->GetResource().Get(), &dsvDesc, dsvDescriptorHeap->HandleCPU(0));
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

