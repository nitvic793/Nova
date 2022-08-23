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
#include <DX12/ContextDX12.h>

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

        mCommandQueue = mDevice.As<DeviceDX12>()->GetCommandQueue();

        for (int i = 0; i < FRAMEBUFFER_COUNT; i++)
        {
            auto hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFences[i].ReleaseAndGetAddressOf()));
            mFenceValues[i] = i == 0 ? 1 : 0;
            mFenceEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }
    }

    void RendererDX12::Destroy()
    {
        mDescriptorHeapPool.Destroy();

        for (int i = 0; i < FRAMEBUFFER_COUNT; ++i)
        {
            uint32_t backBufferIndex = i;
            mCommandQueue->Signal(mFences[backBufferIndex].Get(), mFenceValues[backBufferIndex]);
            if (mFences[backBufferIndex]->GetCompletedValue() < mFenceValues[backBufferIndex])
            {
                auto hr = mFences[backBufferIndex]->SetEventOnCompletion(mFenceValues[backBufferIndex], mFenceEvents[backBufferIndex]);
                WaitForSingleObjectEx(mFenceEvents[backBufferIndex], INFINITE, FALSE);
            }
            CloseHandle(mFenceEvents[i]);
        }
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

        for (int i = 0; i < FRAMEBUFFER_COUNT; i++)
        {
            mContexts[i] = gResourceManager->CreateContext({ .mType = CONTEXT_GFX });
            auto pContext = (ContextDX12*)gResourceManager->GetContext(mContexts[i]);
            if (!pContext->Init(dxDevice->GetDevice(), mCommandAllocators[i].Get()))
            {
                debug::ReportError("Unable to create command context");
            }
            pContext->End();
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
                .mClearValue = {.mFormat = dsvFormat,.mColor = {1.f,0,0,0} , .mStencil = 0, .mIsDepth = true}
            }
        );

        mDepthStencil = gResourceManager->CreateTexture({ .mUsage = tex::USAGE_DEPTH_STENCIL, .mFormat = dsvFormat, .mBuffer = depthResource, .mType = tex::TEXTURE_2D });
    }

    void RendererDX12::Submit(Context* pContext)
    {
        auto context = (ContextDX12*)pContext;
        auto clist = context->GetCommandList();
        
        ID3D12CommandList* ppCommandLists[] = { clist };
        mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    }

    RendererDX12::~RendererDX12()
    {
    }

    void RendererDX12::Wait()
    {
        auto dxDevice = mDevice.As<DeviceDX12>();
        auto pSwapChain = dxDevice->mSwapChain.Get();
        uint32_t backBufferIndex = GetBackBufferIndex();
        const uint64_t currentFenceVal = mFenceValues[backBufferIndex];

        auto hr = mCommandQueue->Signal(mFences[backBufferIndex].Get(), currentFenceVal);
        backBufferIndex = GetBackBufferIndex();
        if (mFences[backBufferIndex]->GetCompletedValue() < mFenceValues[backBufferIndex])
        {
            auto hr = mFences[backBufferIndex]->SetEventOnCompletion(mFenceValues[backBufferIndex], mFenceEvents[backBufferIndex]);
            WaitForSingleObjectEx(mFenceEvents[backBufferIndex], INFINITE, FALSE);
        }

        mFenceValues[backBufferIndex] = currentFenceVal + 1;
    }

    void RendererDX12::ClearBackBuffers()
    {
        const uint32_t backbufferIndex = GetBackBufferIndex();
        auto context = gResourceManager->GetContext(mContexts[backbufferIndex]);

        Viewport viewport = {};
        viewport.mTopLeftX = 0;
        viewport.mTopLeftY = 0;
        viewport.mWidth = (float)gWindow->GetWidth();
        viewport.mHeight = (float)gWindow->GetHeight();
        viewport.mMinDepth = 0.0f;
        viewport.mMaxDepth = 1.0f;

        Rect rect = {};
        rect.mLeft = 0;
        rect.mTop = 0;
        rect.mRight = gWindow->GetWidth();
        rect.mBottom = gWindow->GetHeight();
        
        float clearColor[4] = { 0.f ,0.f, 0.f,0.f };
        context->ClearRenderTarget(mRenderTargets[backbufferIndex], clearColor, 1, &rect);
        context->ClearDepthStencil(mDepthStencil, 1.f, 0, 1, &rect);
    }

    void RendererDX12::TransitionToRenderTarget()
    {
        auto texture = (TextureDX12*)gResourceManager->GetTexture(mRenderTargets[GetBackBufferIndex()]);
        auto context = GetContext();
        TransitionBarrier barrier{ .mFrom = buffer::STATE_PRESENT, .mTo = buffer::STATE_RENDER_TARGET, .mResource = texture->GetBuffer()};
        context->ResourceBarrier({ &barrier , 1 });
    }

    void RendererDX12::TransitionToPresent()
    {
        auto texture = (TextureDX12*)gResourceManager->GetTexture(mRenderTargets[GetBackBufferIndex()]);
        auto context = GetContext();
        TransitionBarrier barrier{ .mFrom = buffer::STATE_RENDER_TARGET, .mTo = buffer::STATE_PRESENT, .mResource = texture->GetBuffer() };
        context->ResourceBarrier({ &barrier , 1 });
    }

    void RendererDX12::StartFrame()
    {
        auto context = GetContext();
        auto pCmdAllocator = GetAllocator();
        //pCmdAllocator->Reset();

        context->Begin();
        TransitionToRenderTarget();
        ClearBackBuffers();
    }

    void RendererDX12::EndFrame()
    {
        auto context = GetContext();
        TransitionToPresent();
        context->End();
        Submit(context);

        uint32_t idx = (GetBackBufferIndex() + 1) % FRAMEBUFFER_COUNT;
        auto pCmdAllocator = mCommandAllocators[idx];
        pCmdAllocator->Reset();
    }

    uint32_t RendererDX12::GetBackBufferIndex() const
    {
        auto dxDevice = mDevice.As<DeviceDX12>();
        auto pSwapChain = dxDevice->mSwapChain.Get();
        return pSwapChain->GetCurrentBackBufferIndex();
    }

    ID3D12CommandAllocator* RendererDX12::GetAllocator() const
    {
        const uint32_t idx = GetBackBufferIndex();
        return mCommandAllocators[idx].Get();
    }

    void RendererDX12::Draw()
    {
    }

    Context* RendererDX12::GetContext() const
    {
        return gResourceManager->GetContext(mContexts[GetBackBufferIndex()]);
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

