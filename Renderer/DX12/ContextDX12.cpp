#include "pch.h"
#include "ContextDX12.h"

#include <Engine/Log.h>

#include <DX12/Interop.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/TextureDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <DX12/MeshDX12.h>
#include <DX12/PipelineStateDX12.h>
#include <DX12/DescriptorHeapDX12.h>

#include <D3D12MemAlloc.h>
#include <d3d12.h>
#include "d3dx12.h"

namespace nv::graphics
{
    namespace util
    {
        // TODO: Move to DX Utils file and dxutils namespace
        static GPUResourceDX12* GetResource(Handle<GPUResource> handle)
        {
            return ((GPUResourceDX12*)gResourceManager->GetGPUResource(handle));
        }

        static ID3D12Resource* GetResource(Handle<Texture> handle)
        {
            return ((TextureDX12*)gResourceManager->GetTexture(handle))->GetResource();
        }
    }

    bool ContextDX12::Init(ID3D12Device* pDevice, ID3D12CommandAllocator* pCommandAllocator)
    {
        auto clistType = GetCommandListType(mDesc.mType);
        auto hr = pDevice->CreateCommandList(0, clistType, pCommandAllocator, nullptr, IID_PPV_ARGS(mCommandList.ReleaseAndGetAddressOf()));

        if (mDesc.mType == CONTEXT_RAYTRACING || true)
            InitRaytracingContext();

        if (FAILED(hr)) return false;

        return true;
    }

    void ContextDX12::Begin(ID3D12CommandAllocator* pCommandAllocator)
    {
        mCommandList->Reset(pCommandAllocator, nullptr);
    }

    void ContextDX12::InitRaytracingContext()
    {
        mDXRCommandList.Reset();
        auto hr = mCommandList->QueryInterface(IID_PPV_ARGS(mDXRCommandList.GetAddressOf()));
        if (FAILED(hr))
        {
            log::Error("Unable to create ray tracing command context.");
        }
    }

    void ContextDX12::Begin()
    {
        auto renderer = (RendererDX12*)gRenderer;
        auto device = ((DeviceDX12*)renderer->GetDevice())->GetDevice();
        auto pCommandAllocator = renderer->GetAllocator();
        
        mCommandList->Reset(pCommandAllocator, nullptr);
    }

    void ContextDX12::End()
    {
        mCommandList->Close();
    }

    void ContextDX12::SetMesh(Handle<Mesh> mesh)
    {
        SetMesh(gResourceManager->GetMesh(mesh));
    }

    void ContextDX12::SetMesh(Mesh* mesh)
    {
        auto dxMesh = (MeshDX12*)mesh;
        const auto& ibv = dxMesh->GetIndexBufferView();
        const auto& vbv = dxMesh->GetVertexBufferView();

        mCommandList->IASetIndexBuffer(&ibv);
        mCommandList->IASetVertexBuffers(0, 1, &vbv);
    }

    void ContextDX12::SetPipeline(Handle<PipelineState> pipeline)
    {
        auto pso = (PipelineStateDX12*)gResourceManager->GetPipelineState(pipeline);
        mCommandList->SetPipelineState(pso->GetPSO());
    }

    void ContextDX12::SetRenderTarget(Span<Handle<Texture>> renderTargets, Handle<Texture> dsvHandle, bool singleRTV)
    {
        nv::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(1);
        for (auto rtv : renderTargets)
        {
            auto tex = (TextureDX12*)gResourceManager->GetTexture(rtv);
            handles.Push(tex->GetCPUHandle());
        }

        TextureDX12* dsvTex = nullptr; 
        if(!dsvHandle.IsNull())
            dsvTex = (TextureDX12*)gResourceManager->GetTexture(dsvHandle);

        D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle;
        if(dsvTex)
            dsvCpuHandle = dsvTex->GetCPUHandle();

        mCommandList->OMSetRenderTargets((UINT)renderTargets.Size(), handles.Data(), singleRTV, dsvTex? &dsvCpuHandle : nullptr);
    }

    void ContextDX12::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
    {
        mCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void ContextDX12::ResourceBarrier(Span<TransitionBarrier> barriers)
    {
        nv::Vector<CD3DX12_RESOURCE_BARRIER> dxBarriers;
        dxBarriers.Reserve((uint32_t)barriers.Size());
        for (auto barrier : barriers)
        {
            auto dxResource = util::GetResource(barrier.mResource);
            ID3D12Resource* pResource = dxResource->GetResource().Get();
            auto from = GetState(dxResource->GetResourceState());
            auto to = GetState(barrier.mTo);
            
            dxResource->UpdateResourceState(barrier.mTo);
            dxBarriers.Push(CD3DX12_RESOURCE_BARRIER::Transition(pResource, from, to));
        }

        mCommandList->ResourceBarrier((UINT)dxBarriers.Size(), dxBarriers.Data());
    }

    void ContextDX12::UpdateSubresources(Handle<GPUResource> dest, Handle<GPUResource> staging, const SubResourceDesc& desc)
    {
        ID3D12Resource* pDest = util::GetResource(dest)->GetResource().Get();
        ID3D12Resource* pStaging = util::GetResource(staging)->GetResource().Get();
        D3D12_SUBRESOURCE_DATA subData = { .pData = desc.mpData, .RowPitch = desc.mRowPitch, .SlicePitch = desc.mSlicePitch };

        ::UpdateSubresources(mCommandList.Get(), pDest, pStaging, desc.mIntermediateOffset, desc.mFirstSubresource, desc.mNumSubresources, &subData);
    }

    void ContextDX12::ClearRenderTarget(Handle<Texture> renderTarget, float color[4], uint32_t numRects, Rect* pRects)
    {
        auto texture = (TextureDX12*)gResourceManager->GetTexture(renderTarget);
        mCommandList->ClearRenderTargetView(texture->GetCPUHandle(), color, numRects, (D3D12_RECT*)pRects);
    }

    void ContextDX12::ClearDepthStencil(Handle<Texture> depthStencil, float depth, uint8_t stencil, uint32_t numRects, Rect* pRects)
    {
        auto texture = (TextureDX12*)gResourceManager->GetTexture(depthStencil);
        mCommandList->ClearDepthStencilView(texture->GetCPUHandle(), D3D12_CLEAR_FLAG_DEPTH, depth, stencil, numRects, (D3D12_RECT*)pRects);
    }

    void ContextDX12::SetViewports(uint32_t numViewports, Viewport* pViewports)
    {
        mCommandList->RSSetViewports(numViewports, (D3D12_VIEWPORT*)pViewports);
    }

    void ContextDX12::SetScissorRect(uint32_t numRect, Rect* pRects)
    {
        mCommandList->RSSetScissorRects(numRect, (D3D12_RECT*)pRects);
    }

    void ContextDX12::SetPrimitiveTopology(PrimitiveTopology topology)
    {
        mCommandList->IASetPrimitiveTopology(GetPrimitiveTopology(topology));
    }

    void ContextDX12::SetDescriptorHeap(Span<Handle<DescriptorHeap>> heaps)
    {
        auto renderer = (RendererDX12*)gRenderer;
        nv::Vector<ID3D12DescriptorHeap*> dxHeaps;
        dxHeaps.Reserve((uint32_t)heaps.Size());
        for (auto heap : heaps)
        {
            ID3D12DescriptorHeap* pHeap = renderer->mDescriptorHeapPool.GetAsDerived(heap)->Get();
            dxHeaps.Push(pHeap);
        }

        mCommandList->SetDescriptorHeaps((UINT)dxHeaps.Size(), dxHeaps.Data());
    }

    void ContextDX12::Bind(uint32_t slot, BindResourceType type, uint32_t offset)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
        auto renderer = (RendererDX12*)gRenderer;
        
        switch (type)
        {
        case BIND_BUFFER:
            handle = renderer->GetConstBufferHandle(offset);
            break;
        case BIND_TEXTURE:
            handle = renderer->GetTextureHandle(offset);
            break;
        }

        mCommandList->SetGraphicsRootDescriptorTable(slot, handle);
    }

    void ContextDX12::BindConstantBuffer(uint32_t slot, uint32_t offset)
    {
        Bind(slot, BIND_BUFFER, offset);
    }

    void ContextDX12::BindTexture(uint32_t slot, Handle<Texture> texture)
    {
        auto tex = (TextureDX12*)gResourceManager->GetTexture(texture);
        Bind(slot, BIND_TEXTURE, tex->GetHeapOffset());
    }

    void ContextDX12::SetRootSignature(ID3D12RootSignature* pRootSig)
    {
        mCommandList->SetGraphicsRootSignature(pRootSig);
    }

    void ContextDX12::SetComputeRootSignature(ID3D12RootSignature* pRootSig)
    {
        mCommandList->SetComputeRootSignature(pRootSig);
    }

    void ContextDX12::Dispatch(uint32_t x, uint32_t y, uint32_t z)
    {
        mCommandList->Dispatch(x, y, z);
    }

    void ContextDX12::ComputeBind(uint32_t slot, BindResourceType type, uint32_t offset)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
        auto renderer = (RendererDX12*)gRenderer;

        switch (type)
        {
        case BIND_BUFFER:
            handle = renderer->GetConstBufferHandle(offset);
            break;
        case BIND_TEXTURE:
            handle = renderer->GetTextureHandle(offset);
            break;
        }

        mCommandList->SetComputeRootDescriptorTable(slot, handle);
    }

    void ContextDX12::ComputeBindConstantBuffer(uint32_t slot, uint32_t offset)
    {
        Bind(slot, BIND_BUFFER, offset);
    }

    void ContextDX12::ComputeBindTexture(uint32_t slot, Handle<Texture> texture)
    {
        auto tex = (TextureDX12*)gResourceManager->GetTexture(texture);
        Bind(slot, BIND_TEXTURE, tex->GetHeapOffset());
    }
}