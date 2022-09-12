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
#include <DX12/ConstantBuffer.h>

#include <DX12/DirectXIncludes.h>
#include <dxgidebug.h>
#include <D3D12MemAlloc.h>

#include <Debug/Profiler.h>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace nv::graphics
{
    constexpr uint64_t MAX_CONST_BUFFER_SIZE = 1024 * 256; // 256KB
    constexpr uint32_t kDefaultDescriptorCount = 32;
    constexpr uint32_t kDefaultFrameDescriptorCount = 1024;
    constexpr uint32_t kDefaultGPUDescriptorCount = kDefaultFrameDescriptorCount * 3;

    void RendererDX12::Init(Window& window)
    {
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

        NV_GPU_INIT_D3D12(pDevice, mCommandQueue.GetAddressOf(), 1);

        for (int i = 0; i < FRAMEBUFFER_COUNT; i++)
        {
            auto hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFences[i].ReleaseAndGetAddressOf()));
            mFenceValues[i] = i == 0 ? 1 : 0;
            mFenceEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }

        CreateRootSignature();
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
        mBackbufferFormat = format;
        mDsvFormat = format::D32_FLOAT;
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
        
        auto depthResource = gResourceManager->CreateResource(
            {
                .mWidth = gWindow->GetWidth(),
                .mHeight = gWindow->GetHeight(),
                .mFormat = mDsvFormat,
                .mType = buffer::TYPE_TEXTURE_2D,
                .mFlags = buffer::FLAG_ALLOW_DEPTH,
                .mInitialState = buffer::STATE_DEPTH_WRITE,
                .mClearValue = {.mFormat = mDsvFormat,.mColor = {1.f,0,0,0} , .mStencil = 0, .mIsDepth = true}
            }
        );

        mDepthStencil = gResourceManager->CreateTexture({ .mUsage = tex::USAGE_DEPTH_STENCIL, .mFormat = mDsvFormat, .mBuffer = depthResource, .mType = tex::TEXTURE_2D });

        mConstantBuffer = Alloc<GPUConstantBuffer>();
        mConstantBuffer->Initialize(MAX_CONST_BUFFER_SIZE);
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
        Free(mConstantBuffer);
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
        
        float clearColor[4] = { 0.393f ,0.585f, 0.93f, 1.f };
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
        auto context = (ContextDX12*)GetContext();
        auto pCmdAllocator = GetAllocator();
        //pCmdAllocator->Reset();

        mGpuHeapState.Reset();
        CopyDescriptorsToGPU();

        context->Begin();
        context->SetRootSignature(mRootSignature.Get());
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

    ID3D12CommandQueue* RendererDX12::GetCommandQueue() const
    {
        return mCommandQueue.Get();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE RendererDX12::GetConstBufferHandle(uint32_t index) const
    {
        auto heap = mDescriptorHeapPool.GetAsDerived(mGpuHeap);
        return heap->HandleGPU(mGpuHeapState.mConstBufferOffset + index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE RendererDX12::GetTextureHandle(uint32_t index) const
    {
        auto heap = mDescriptorHeapPool.GetAsDerived(mGpuHeap);
        return heap->HandleGPU(mGpuHeapState.mTextureOffset + index);
    }

    ConstantBufferView RendererDX12::CreateConstantBuffer(uint32_t size)
    {
        auto heap = mDescriptorHeapPool.GetAsDerived(mConstantBufferHeap);
        auto pDevice = mDevice.As<DeviceDX12>()->GetDevice();
        const uint32_t bufferSize = (size + CONST_BUFFER_ALIGNMENT_SIZE - 1) & ~(CONST_BUFFER_ALIGNMENT_SIZE - 1);
        const uint32_t cbIndex = mCbState.mCurrentCount;
        const uint64_t cbMemOffset = mCbState.mCurrentMemoryOffset;

        mCbState.mCurrentCount++;
        mCbState.mCurrentMemoryOffset += bufferSize;

        D3D12_CONSTANT_BUFFER_VIEW_DESC	desc = { .BufferLocation = mConstantBuffer->GetAddress() + cbMemOffset, .SizeInBytes = bufferSize };
        pDevice->CreateConstantBufferView(&desc, heap->HandleCPU(cbIndex));
        heap->PushCPU();

        return ConstantBufferView{ cbMemOffset, cbIndex };
    }

    void RendererDX12::UploadToConstantBuffer(ConstantBufferView view, uint8_t* data, uint32_t size)
    {
        mConstantBuffer->CopyDataOffset((void*)data, size, view.mMemoryOffset);
    }

    void RendererDX12::CreateRootSignature()
    {
        enum RootParameterSlot 
        {
            RootSigCBVertex0 = 0,
            RootSigComputeCB = 0,
            RootSigCBPixel0,
            RootSigComputeUAV = 1,
            RootSigSRVPixel1,
            RootSigComputeSRV = 2,
            RootSigSRVPixel2,
            RootSigCBAll1,
            RootSigCBAll2,
            RootSigIBL,
            RootSigUAV0,
            RootSigParamCount
        };

        auto device = mDevice.As<DeviceDX12>()->GetDevice();
        CD3DX12_DESCRIPTOR_RANGE range[RootSigParamCount] = {};
        //view dependent CBV
        range[RootSigCBVertex0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
        //light dependent CBV
        range[RootSigCBPixel0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
        //G-Buffer inputs
        range[RootSigSRVPixel1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0);
        //Extra Textures
        range[RootSigSRVPixel2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 8);
        //per frame CBV
        range[RootSigCBAll1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
        //per bone 
        range[RootSigCBAll2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
        //IBL Textures
        range[RootSigIBL].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 16);
        // UAV textures
        range[RootSigUAV0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0);

        CD3DX12_ROOT_PARAMETER rootParameters[RootSigParamCount] = {};
        rootParameters[RootSigCBVertex0].InitAsDescriptorTable(1, &range[RootSigCBVertex0], D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[RootSigCBPixel0].InitAsDescriptorTable(1, &range[RootSigCBPixel0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootSigSRVPixel1].InitAsDescriptorTable(1, &range[RootSigSRVPixel1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootSigSRVPixel2].InitAsDescriptorTable(1, &range[RootSigSRVPixel2], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootSigCBAll1].InitAsDescriptorTable(1, &range[RootSigCBAll1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[RootSigCBAll2].InitAsDescriptorTable(1, &range[RootSigCBAll2], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[RootSigIBL].InitAsDescriptorTable(1, &range[RootSigIBL], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootSigUAV0].InitAsDescriptorTable(1, &range[RootSigUAV0], D3D12_SHADER_VISIBILITY_ALL);

        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
        descRootSignature.Init(RootSigParamCount, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS);

        CD3DX12_STATIC_SAMPLER_DESC staticSamplers[5] = {};
        //Base Sampler
        staticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.f, 4);
        //Shadow Sampler
        staticSamplers[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            0.f, 16u, D3D12_COMPARISON_FUNC_LESS_EQUAL);

        staticSamplers[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP);

        staticSamplers[3].Init(3, D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        staticSamplers[4].Init(4, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        Microsoft::WRL::ComPtr<ID3DBlob> rootSigBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);
        device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf()));
    }

    void RendererDX12::CopyDescriptorsToGPU()
    {
        auto pDevice = mDevice.As<DeviceDX12>()->GetDevice();
        auto gpuHeap = mDescriptorHeapPool.GetAsDerived(mGpuHeap);
        const uint32_t frameOffset = GetBackBufferIndex() * kDefaultFrameDescriptorCount;
        auto copyDescriptors = [&](Handle<DescriptorHeap> handle)
        {
            auto heap = mDescriptorHeapPool.GetAsDerived(handle);
            constexpr auto descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            const uint32_t size = heap->GetSize();
            const uint32_t offset = frameOffset + mGpuHeapState.mCurrentCount;
            if(size > 0)
                pDevice->CopyDescriptorsSimple(size, gpuHeap->HandleCPU(offset), heap->HandleCPU(0), descriptorType);
            mGpuHeapState.mCurrentCount += size;
            return offset;
        };

        mGpuHeapState.mConstBufferOffset = copyDescriptors(mConstantBufferHeap);
        mGpuHeapState.mTextureOffset = copyDescriptors(mTextureHeap);
    }

    void RendererDX12::Draw()
    {
    }

    Handle<Texture> RendererDX12::GetDefaultDepthTarget() const
    {
        return mDepthStencil;
    }

    Handle<Texture> RendererDX12::GetDefaultRenderTarget() const
    {
        const auto idx = GetBackBufferIndex();
        return mRenderTargets[idx];
    }

    Handle<DescriptorHeap> RendererDX12::GetGPUDescriptorHeap() const
    {
        return mGpuHeap;
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

