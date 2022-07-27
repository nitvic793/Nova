#include "pch.h"
#include "ContextDX12.h"
#include <DX12/Interop.h>
#include <DX12/RendererDX12.h>
#include <DX12/DeviceDX12.h>
#include <d3d12.h>

namespace nv::graphics
{
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
    }

    void ContextDX12::SetPipeline(Handle<PipelineState> pipeline)
    {
    }

    void ContextDX12::SetRenderTarget(Span<Handle<Texture>> renderTargets, Handle<Texture> dsvHandle, bool singleRTV)
    {
    }

    void ContextDX12::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
    {
        mCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void ContextDX12::ResourceBarrier(Span<TransitionBarrier> barriers)
    {
    }

}