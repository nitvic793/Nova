#pragma once

#include <Lib/Handle.h>
#include <Renderer/Mesh.h>
#include <d3d12.h>

namespace nv::graphics
{
    class GPUResource;
    class Texture;

    class MeshDX12 : public Mesh
    {
    public:
        MeshDX12() :
            Mesh({}),
            mVertexBuffer(),
            mIndexBuffer(),
            mIndexBufferView(),
            mVertexBufferView(),
            mBoneBufferView()
        {}

        MeshDX12(const MeshDesc& desc, Handle<GPUResource> vertexBuffer, Handle<GPUResource> indexBuffer, Handle<GPUResource> boneBuffer);

        const D3D12_INDEX_BUFFER_VIEW&  GetIndexBufferView() const { return mIndexBufferView; }
        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return mVertexBufferView; }
        const D3D12_VERTEX_BUFFER_VIEW& GetBoneBufferView() const { return mBoneBufferView; }
        const std::vector<MeshEntry>&   GetMeshEntries() const { return mDesc.mMeshEntries; }

        void                            GenerateBufferSRVs();
        D3D12_RAYTRACING_GEOMETRY_DESC  GetGeometryDescs();
        ID3D12Resource*                 GetVertexBuffer() const;
        ID3D12Resource*                 GetIndexBuffer() const;
        Texture*                        GetIndexBufferSRV() const;
        Texture*                        GetVertexBufferSRV() const;

    private:
        Handle<GPUResource>         mVertexBuffer;
        Handle<GPUResource>         mIndexBuffer;
        D3D12_INDEX_BUFFER_VIEW     mIndexBufferView;
        D3D12_VERTEX_BUFFER_VIEW    mVertexBufferView;
        D3D12_VERTEX_BUFFER_VIEW    mBoneBufferView;
        Handle<Texture>             mIndexBufferSRV;
        Handle<Texture>             mVertexBufferSRV;
    };
}