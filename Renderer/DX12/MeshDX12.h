#pragma once

#include <Lib/Handle.h>
#include <Renderer/Mesh.h>
#include <d3d12.h>

//struct ID3D12Resource;
//struct D3D12_VERTEX_BUFFER_VIEW;
//struct D3D12_INDEX_BUFFER_VIEW;

namespace nv::graphics
{
    class GPUResource;

    class MeshDX12 : public Mesh
    {
    public:
        MeshDX12() :
            Mesh({}),
            mVertexBuffer(),
            mIndexBuffer(),
            mIndexBufferView(),
            mVertexBufferView()
        {}

        MeshDX12(const MeshDesc& desc, Handle<GPUResource> vertexBuffer, Handle<GPUResource> indexBuffer);

        const D3D12_INDEX_BUFFER_VIEW&  GetIndexBufferView() const { return mIndexBufferView; }
        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return mVertexBufferView; }
        const std::vector<MeshEntry>&   GetMeshEntries() const { return mDesc.mMeshEntries; }

    private:
        Handle<GPUResource> mVertexBuffer;
        Handle<GPUResource> mIndexBuffer;
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    };
}