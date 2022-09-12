#include "pch.h"
#include "MeshDX12.h"

#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <D3D12MemAlloc.h>

namespace nv::graphics
{
    MeshDX12::MeshDX12(const MeshDesc& desc, Handle<GPUResource> vertexBuffer, Handle<GPUResource> indexBuffer):
        Mesh(desc),
        mVertexBuffer(vertexBuffer),
        mIndexBuffer(indexBuffer),
        mIndexBufferView(),
        mVertexBufferView()
    {
        auto vb = (GPUResourceDX12*)gResourceManager->GetGPUResource(vertexBuffer);
        auto ib = (GPUResourceDX12*)gResourceManager->GetGPUResource(indexBuffer);

        const uint32_t indexBufferSize = sizeof(uint32_t) * (uint32_t)desc.mIndices.size();
        const uint32_t vertexBufferSize = sizeof(Vertex) * (uint32_t)desc.mVertices.size();

        mVertexBufferView.BufferLocation = vb->GetResource()->GetGPUVirtualAddress();
        mVertexBufferView.StrideInBytes = sizeof(Vertex);
        mVertexBufferView.SizeInBytes = vertexBufferSize;

        mIndexBufferView.BufferLocation = ib->GetResource()->GetGPUVirtualAddress();
        mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
        mIndexBufferView.SizeInBytes = indexBufferSize;
    }
}

