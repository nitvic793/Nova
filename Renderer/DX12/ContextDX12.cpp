#include "pch.h"
#include "ContextDX12.h"
#include <DX12/Interop.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/TextureDX12.h>
#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <D3D12MemAlloc.h>
#include <d3d12.h>
#include "d3dx12.h"

namespace nv::graphics
{
    namespace util
    {
        // TODO: Move to DX Utils file and dxutils namespace
        static ID3D12Resource* GetResource(Handle<GPUResource> handle)
        {
            return ((GPUResourceDX12*)gResourceManager->GetGPUResource(handle))->GetResource().Get();
        }
    }

    bool ContextDX12::Init(ID3D12Device* pDevice, ID3D12CommandAllocator* pCommandAllocator)
    {
        auto clistType = GetCommandListType(mDesc.mType);
        auto hr = pDevice->CreateCommandList(0, clistType, pCommandAllocator, nullptr, IID_PPV_ARGS(mCommandList.ReleaseAndGetAddressOf()));
        if (FAILED(hr)) return false;

        return true;
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
        auto dxMesh = (MeshDX12*)gResourceManager->GetMesh(mesh);
    }

    void ContextDX12::SetPipeline(Handle<PipelineState> pipeline)
    {
        auto pipeState = (PipelineStateDX12*)gResourceManager->GetPipelineState(pipeline);
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
        dxBarriers.Reserve(barriers.Size());
        for (auto barrier : barriers)
        {
            ID3D12Resource* pResource = util::GetResource(barrier.mResource);
            auto from = GetState(barrier.mFrom);
            auto to = GetState(barrier.mTo);
            dxBarriers.Push(CD3DX12_RESOURCE_BARRIER::Transition(pResource, from, to));
        }

        mCommandList->ResourceBarrier((UINT)dxBarriers.Size(), dxBarriers.Data());
    }

    void ContextDX12::UpdateSubresources(Handle<GPUResource> dest, Handle<GPUResource> staging, const SubResourceDesc& desc)
    {
        ID3D12Resource* pDest = util::GetResource(dest);
        ID3D12Resource* pStaging = util::GetResource(staging);
        D3D12_SUBRESOURCE_DATA subData = { .pData = desc.mpData, .RowPitch = desc.mRowPitch, .SlicePitch = desc.mSlicePitch };

        ::UpdateSubresources(mCommandList.Get(), pDest, pStaging, desc.mIntermediateOffset, desc.mFirstSubresource, desc.mNumSubresources, &subData);
    }

}