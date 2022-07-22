#include "pch.h"

#include <Debug/Error.h>

#include "RendererDX12.h"
#include <Renderer/DescriptorHeap.h>
#include <DX12/DeviceDX12.h>
#include <DX12/WindowDX12.h>
#include <DX12/DescriptorHeapDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/Interop.h>
#include <DX12/TextureDX12.h>

#include <DX12/DirectXIncludes.h>
#include <dxgidebug.h>
#include <D3D12MemAlloc.h>

namespace nv::graphics
{
    void RendererDX12::Init(Window& window)
    {
        constexpr uint32_t kDefaultDescriptorCount = 32;
        constexpr uint32_t kDefaultGPUDescriptorCount = 1024;

        mDescriptorHeapPool.Init();
        mDevice = ScopedPtr<Device, true>((Device*)Alloc<DeviceDX12>());
        mDevice->Init(window);

        auto pDevice = mDevice.As<DeviceDX12>()->GetDevice();

        auto initDescriptorHeap = [&](D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorCount, Handle<DescriptorHeap>& handle, bool shaderVisible = false)
        {
            auto descriptorHeap = (DescriptorHeapDX12*)mDescriptorHeapPool.CreateInstance(handle);
            descriptorHeap->Create(pDevice, type, descriptorCount, shaderVisible);
            return descriptorHeap;
        };

        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kDefaultDescriptorCount, mRtvHeap);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kDefaultDescriptorCount, mDsvHeap);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDefaultGPUDescriptorCount, mGpuHeap, true);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDefaultDescriptorCount, mTextureHeap);
        initDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDefaultDescriptorCount, mConstantBufferHeap);

        for (uint32_t i = 0; i < FRAMEBUFFER_COUNT; ++i)
        {
            pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCommandAllocators[i].ReleaseAndGetAddressOf()));
        }
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

        auto dxDevice = mDevice.As<DeviceDX12>();
        auto rtvDescriptorHeap = mDescriptorHeapPool.GetAsDerived(mRtvHeap);
        auto dsvDescriptorHeap = mDescriptorHeapPool.GetAsDerived(mDsvHeap);

        auto rm = (ResourceManagerDX12*)gResourceManager;
        for (int32_t i = 0; i < FRAMEBUFFER_COUNT; ++i)
            backBufferResources[i] = rm->mGpuResources.CreateInstance(mpBackBuffers[i], GPUResourceDesc{.mFormat = format /*TODO: Translate all*/});

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
            TextureDesc rtvDesc = { .mUsage = tex::USAGE_RENDER_TARGET, .mFormat = format,.mBuffer = mpBackBuffers[i] };
            mRenderTargets[i] = gResourceManager->CreateTexture(rtvDesc);
        }

        const auto dsvFormat = format::D32_FLOAT;
        
        auto depthResource = gResourceManager->CreateResource(
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

        mDepthStencil = gResourceManager->CreateTexture({ .mUsage = tex::USAGE_DEPTH_STENCIL, .mFormat = dsvFormat, .mBuffer = depthResource, .mType = tex::TEXTURE_2D });
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

