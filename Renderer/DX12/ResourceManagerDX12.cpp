#include "pch.h"

#include "ResourceManagerDX12.h"
#include <Engine/Log.h>

#include <DX12/RendererDX12.h>
#include <DX12/ShaderDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/PipelineStateDX12.h>
#include <DX12/TextureDX12.h>
#include <DX12/MeshDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/Interop.h>
#include <DX12/DescriptorHeapDX12.h>
#include <DX12/ContextDX12.h>

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <DirectXHelpers.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

namespace nv::graphics
{
    // Creates a context with RAII. Useful to create a local context especially for copy workloads
    // Not meant to be re-used during main rendering as the command allocator 
    // is reset when after submit and wait. 
    class AutoLocalContext
    {
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        AutoLocalContext(ID3D12Device* pDevice, ContextType contextType, ID3D12CommandQueue* pCmdQueue):
            mpCommandQueue(pCmdQueue)
        {
            const D3D12_COMMAND_LIST_TYPE type = GetCommandListType(contextType);
            auto hr = pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(mCommandAllocator.ReleaseAndGetAddressOf()));
            hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.ReleaseAndGetAddressOf()));

            mFenceValue = 1;
            mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            mContext = ContextDX12({ .mType = contextType });
            mContext.Init(pDevice, mCommandAllocator.Get());
            //mContext.Begin(mCommandAllocator.Get());
        }

        ID3D12GraphicsCommandList4* GetCommandList() const 
        { 
            return mContext.GetCommandList(); 
        }

        ContextDX12* GetContext()
        {
            return &mContext;
        }

        void Reset()
        {
            mContext.Begin(mCommandAllocator.Get());
        }

        void SubmitAndWait()
        {
            mContext.End();
            ID3D12CommandList* ppCommandLists[] = { mContext.GetCommandList() };
            mpCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

            auto hr = mpCommandQueue->Signal(mFence.Get(), mFenceValue);
            if (mFence->GetCompletedValue() < mFenceValue)
            {
                auto hr = mFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
                WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
            }

            mCommandAllocator->Reset();
            mFenceValue++;
        }

        ~AutoLocalContext()
        {
            SubmitAndWait();
            CloseHandle(mFenceEvent);
        }

    private:
        ContextDX12                     mContext;
        ComPtr<ID3D12CommandAllocator>  mCommandAllocator;
        ComPtr<ID3D12Fence>             mFence;
        uint64_t                        mFenceValue;
        HANDLE                          mFenceEvent;
        ID3D12CommandQueue*             mpCommandQueue;

    };

    ResourceManagerDX12::ResourceManagerDX12()
    {
        mDevice = (DeviceDX12*)gRenderer->GetDevice();
        mGpuResources.Init();
        mMeshes.Init();
        mPipelineStates.Init();
        mShaders.Init();
        mTextures.Init();
        mContexts.Init();
    }

    Handle<Shader> ResourceManagerDX12::CreateShader(const ShaderDesc& desc)
    {
        return Handle<Shader>();
    }

    Handle<GPUResource> ResourceManagerDX12::CreateResource(const GPUResourceDesc& desc)
    {
        ID3D12Device* device = mDevice->GetDevice();
        auto pAllocator = mDevice->GetAllocator();

        assert(pAllocator);
        
        Handle<GPUResource> handle;
        auto resource = (GPUResourceDX12*)mGpuResources.CreateInstance(handle, desc);
        if (!resource)
            return Null<GPUResource>();
        
        ID3D12Resource* d3dResource = nullptr;
        CD3DX12_RESOURCE_DESC bufferDesc = {};
        D3D12_HEAP_FLAGS heapFlag = D3D12_HEAP_FLAG_NONE;
        D3D12_CLEAR_VALUE clearValue = {};

        const D3D12MA::ALLOCATION_DESC allocationDesc = { .HeapType = GetHeapType(desc.mBufferMode) };
        const CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(GetHeapType(desc.mBufferMode));
        const D3D12_RESOURCE_STATES initResourceState = GetState(desc.mInitialState);
        const DXGI_FORMAT format = GetFormat(desc.mFormat);
        const D3D12_RESOURCE_FLAGS flags = GetFlags(desc.mFlags);

        if (desc.mClearValue.mIsDepth)
            clearValue = CD3DX12_CLEAR_VALUE(GetFormat(desc.mClearValue.mFormat), desc.mClearValue.mColor[0], desc.mClearValue.mStencil);
        else
            clearValue = CD3DX12_CLEAR_VALUE(GetFormat(desc.mClearValue.mFormat), desc.mClearValue.mColor);

        D3D12_CLEAR_VALUE* outClearValue = &clearValue;
       
        std::wstring resourceName;
        switch (desc.mType)
        {
        case buffer::TYPE_BUFFER:
            bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.mWidth);
            resourceName = L"Buffer";
            outClearValue = nullptr;
            break;
        case buffer::TYPE_TEXTURE_2D: 
            bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, desc.mWidth, desc.mHeight, desc.mArraySize, desc.mMipLevels, desc.mSampleCount, desc.mSampleQuality, flags);
            resourceName = L"Texture2D";
            break;
        }

        D3D12MA::Allocation* allocation = nullptr;
        auto hr = pAllocator->CreateResource(&allocationDesc, &bufferDesc, initResourceState, outClearValue, &allocation, IID_NULL, nullptr);
        resource->SetResource(allocation);

        allocation->GetResource()->SetName(resourceName.c_str());
        
        if (!SUCCEEDED(hr)) return Null<GPUResource>();

        return handle;
    }

    Handle<PipelineState> ResourceManagerDX12::CreatePipelineState(const PipelineStateDesc& desc)
    {
        return Handle<PipelineState>();
    }

    Handle<Texture> ResourceManagerDX12::CreateTexture(const TextureDesc& desc)
    {
        assert(mGpuResources.IsValid(desc.mBuffer));
        assert(gRenderer);
        assert(mDevice);

        Handle<Texture> handle;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
        DescriptorViews view = VIEW_NONE;
        DescriptorHeapDX12* heap = nullptr;
        auto renderer = (RendererDX12*)gRenderer;
        auto resource = mGpuResources.GetAsDerived(desc.mBuffer);
        auto device = mDevice->GetDevice();
        const auto resourceDesc = resource->GetDesc();

        DXGI_FORMAT dxgiFormat = GetFormat(resourceDesc.mFormat == format::UNKNOWN ? desc.mFormat : resourceDesc.mFormat);

        switch (desc.mUsage)
        {
        case tex::USAGE_SHADER:
        {
            view = VIEW_SHADER_RESOURCE;
            heap = renderer->mDescriptorHeapPool.GetAsDerived(renderer->mTextureHeap);
            cpuHandle = heap->PushCPU();
            DirectX::CreateShaderResourceView(device, resource->GetResource().Get(), cpuHandle, tex::TEXTURE_CUBE == desc.mType);
            break;
        }
        case tex::USAGE_UNORDERED:
        {
            view = VIEW_UNORDERED_ACCESS;
            heap = renderer->mDescriptorHeapPool.GetAsDerived(renderer->mTextureHeap);
            cpuHandle = heap->PushCPU();

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = GetUAVDimension(desc.mType);
            uavDesc.Format = dxgiFormat;
            device->CreateUnorderedAccessView(resource->GetResource().Get(), nullptr, &uavDesc, cpuHandle);
            break;
        }
        case tex::USAGE_DEPTH_STENCIL:
        {
            view = VIEW_DEPTH_STENCIL;
            heap = renderer->mDescriptorHeapPool.GetAsDerived(renderer->mDsvHeap);
            cpuHandle = heap->PushCPU();

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Texture2D.MipSlice = 0;
            dsvDesc.Format = dxgiFormat;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            device->CreateDepthStencilView(resource->GetResource().Get(), &dsvDesc, cpuHandle);
            break;
        }
        case tex::USAGE_RENDER_TARGET:
        {
            view = VIEW_RENDER_TARGET;
            heap = renderer->mDescriptorHeapPool.GetAsDerived(renderer->mRtvHeap);
            cpuHandle = heap->PushCPU();

            const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
            {
                .Format = dxgiFormat,
                .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
                .Texture2D = {.MipSlice = 0, .PlaneSlice = 0},
            };
            device->CreateRenderTargetView(resource->GetResource().Get(), &rtvDesc, cpuHandle);
            break;
        }
        default:
            break;
        }

        assert(heap);
        DescriptorHandle descHandle(cpuHandle);
        descHandle.mView = view;
        descHandle.mType = DescriptorHandle::CPU;
        if(heap)
            descHandle.mHeapIndex = heap->GetCurrentIndex();
        auto texture = mTextures.CreateInstance(handle, desc, descHandle);
        
        return handle;
    }

    Handle<Mesh> ResourceManagerDX12::CreateMesh(const MeshDesc& desc)
    {
        Handle<Mesh> handle;
        const uint32_t indexBufferSize = sizeof(uint32_t) * (uint32_t)desc.mIndices.size();
        const uint32_t vertexBufferSize = sizeof(Vertex) * (uint32_t)desc.mVertices.size();

        Handle<GPUResource> indexBuffer = CreateResource({ .mSize = indexBufferSize, .mType = buffer::TYPE_BUFFER, .mInitialState = buffer::STATE_COPY_DEST});
        Handle<GPUResource> indexBufferUpload = CreateResource({ .mSize = indexBufferSize, .mType = buffer::TYPE_BUFFER, .mInitialState = buffer::STATE_GENERIC_READ, .mBufferMode = buffer::BUFFER_MODE_UPLOAD });

        Handle<GPUResource> vertexBuffer = CreateResource({ .mSize = vertexBufferSize, .mType = buffer::TYPE_BUFFER, .mInitialState = buffer::STATE_COPY_DEST });
        Handle<GPUResource> vertexBufferUpload = CreateResource({ .mSize = vertexBufferSize, .mType = buffer::TYPE_BUFFER, .mInitialState = buffer::STATE_GENERIC_READ, .mBufferMode = buffer::BUFFER_MODE_UPLOAD });

        auto ibResource = (GPUResourceDX12*)this->GetGPUResource(indexBuffer);
        auto vbResource = (GPUResourceDX12*)this->GetGPUResource(vertexBuffer);

        auto ibUploadResource = (GPUResourceDX12*)this->GetGPUResource(indexBufferUpload);
        auto vbUploadResource = (GPUResourceDX12*)this->GetGPUResource(vertexBufferUpload);

        auto pDevice = ((DeviceDX12*)gRenderer->GetDevice())->GetDevice();
        UploadData ibData = 
        {
            .mpData = reinterpret_cast<const BYTE*>(desc.mIndices.data()),
            .mRowPitch = indexBufferSize,
            .mSlicePitch = indexBufferSize
        };

        UploadData vbData =
        {
            .mpData = reinterpret_cast<const BYTE*>(desc.mVertices.data()),
            .mRowPitch = vertexBufferSize,
            .mSlicePitch = vertexBufferSize
        };
        
        {
            AutoLocalContext localContext(pDevice, ContextType::CONTEXT_GFX, ((RendererDX12*)(gRenderer))->GetCommandQueue());
            ibResource->UploadResource(localContext.GetCommandList(), ibData, ibUploadResource->GetResource().Get());
            vbResource->UploadResource(localContext.GetCommandList(), vbData, vbUploadResource->GetResource().Get());

            auto ctx = localContext.GetContext();
            TransitionBarrier barriers[] = 
            {
                {.mFrom = buffer::STATE_COPY_DEST, .mTo = buffer::STATE_INDEX_BUFFER, .mResource = indexBuffer },
                {.mFrom = buffer::STATE_COPY_DEST, .mTo = buffer::STATE_VERTEX_BUFFER, .mResource = vertexBuffer }
            };

            ctx->ResourceBarrier({ barriers, _countof(barriers) });
        }

        mGpuResources.Remove(vertexBufferUpload);
        mGpuResources.Remove(indexBufferUpload);

        MeshDX12* mesh = mMeshes.CreateInstance(handle, desc, vertexBuffer, indexBuffer);
        return handle;
    }

    Handle<Context> ResourceManagerDX12::CreateContext(const ContextDesc& desc)
    {
        Handle<Context> handle;
        auto context = (ContextDX12*)mContexts.CreateInstance(handle, desc);
        return handle;
    }

    GPUResource* ResourceManagerDX12::Emplace(Handle<GPUResource>& handle)
    {
        return mGpuResources.CreateInstance(handle);
    }

    Texture* ResourceManagerDX12::GetTexture(Handle<Texture> handle)
    {
        return mTextures.Get(handle);
    }

    GPUResource* ResourceManagerDX12::GetGPUResource(Handle<GPUResource> handle)
    {
        return mGpuResources.Get(handle);
    }
    
    PipelineState* ResourceManagerDX12::GetPipelineState(Handle<PipelineState> handle)
    {
        return mPipelineStates.Get(handle);
    }

    Shader* ResourceManagerDX12::GetShader(Handle<Shader> handle)
    {
        return mShaders.Get(handle);
    }

    Mesh* ResourceManagerDX12::GetMesh(Handle<Mesh> handle)
    {
        return mMeshes.Get(handle);
    }

    Context* ResourceManagerDX12::GetContext(Handle<Context> handle)
    {
        return mContexts.Get(handle);
    }

    ResourceManagerDX12::~ResourceManagerDX12()
    {
        mMeshes.Destroy();
        mGpuResources.Destroy();
        mPipelineStates.Destroy();
        mShaders.Destroy();
        mTextures.Destroy();
        mContexts.Destroy();
    }
}
