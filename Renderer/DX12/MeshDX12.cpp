#include "pch.h"
#include "MeshDX12.h"

#include <DX12/ResourceManagerDX12.h>
#include <DX12/GPUResourceDX12.h>
#include <D3D12MemAlloc.h>
#include <DX12/TextureDX12.h>

namespace nv::graphics
{
    MeshDX12::MeshDX12(const MeshDesc& desc, Handle<GPUResource> vertexBuffer, Handle<GPUResource> indexBuffer, Handle<GPUResource> boneBuffer):
        Mesh(desc),
        mVertexBuffer(vertexBuffer),
        mIndexBuffer(indexBuffer),
        mIndexBufferView(),
        mVertexBufferView(),
        mBoneBufferView()
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

        if (!boneBuffer.IsNull())
        {
            auto res = (GPUResourceDX12*)gResourceManager->GetGPUResource(boneBuffer);
            const uint32_t boneBufferSize = sizeof(VertexBoneData) * (uint32_t)desc.mBoneDesc.mBones.size();

            mBoneBufferView.BufferLocation = res->GetResource()->GetGPUVirtualAddress();
            mBoneBufferView.StrideInBytes = sizeof(VertexBoneData);
            mBoneBufferView.SizeInBytes = boneBufferSize;
        }
    }

    void MeshDX12::GenerateBufferSRVs() 
    {
        auto createBufferSrv = [&](Handle<Texture>& outTex, Handle<GPUResource> buffer, uint32_t numElements, uint32_t elementsSize)
        {
            TextureDesc desc =
            {
                .mUsage = tex::USAGE_SHADER,
                .mBuffer = buffer,
                .mType = tex::BUFFER,
            };
            
            format::SurfaceFormat format;
            tex::Buffer& bufferData = desc.mBufferData;
            bufferData.mNumElements = numElements;
            if (elementsSize == 0)
            {
                format = format::R32_TYPELESS;
                bufferData.mBufferFlags = tex::BUFFER_FLAG_RAW;
                bufferData.mStructureByteStride = 0;
            }
            else
            {
                format = format::FORMAT_UNKNOWN;
                bufferData.mBufferFlags = tex::BUFFER_FLAG_NONE;
                bufferData.mStructureByteStride = elementsSize;
            }

            outTex = gResourceManager->CreateTexture(desc);
        };

        createBufferSrv(mIndexBufferSRV, mIndexBuffer, (uint32_t)mDesc.mIndices.size(), sizeof(uint32_t));
        createBufferSrv(mVertexBufferSRV, mVertexBuffer, (uint32_t)mDesc.mVertices.size(), sizeof(Vertex));
    }

    D3D12_RAYTRACING_GEOMETRY_DESC MeshDX12::GetGeometryDescs()
    {
        auto vb = (GPUResourceDX12*)gResourceManager->GetGPUResource(mVertexBuffer);
        auto ib = (GPUResourceDX12*)gResourceManager->GetGPUResource(mIndexBuffer);

        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometryDesc.Triangles.IndexBuffer = ib->GetResource()->GetGPUVirtualAddress();
        geometryDesc.Triangles.IndexCount = static_cast<UINT>(ib->GetResource()->GetDesc().Width) / sizeof(uint32_t);
        geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
        geometryDesc.Triangles.Transform3x4 = 0;
        geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geometryDesc.Triangles.VertexCount = static_cast<UINT>(vb->GetResource()->GetDesc().Width) / sizeof(Vertex);
        geometryDesc.Triangles.VertexBuffer.StartAddress = vb->GetResource()->GetGPUVirtualAddress();
        geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
        geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        return geometryDesc;
    }

    ID3D12Resource* MeshDX12::GetVertexBuffer() const
    {
        auto resource = (GPUResourceDX12*)gResourceManager->GetGPUResource(mVertexBuffer);
        return resource->GetResource().Get();
    }

    ID3D12Resource* MeshDX12::GetIndexBuffer() const
    {
        auto resource = (GPUResourceDX12*)gResourceManager->GetGPUResource(mIndexBuffer);
        return resource->GetResource().Get();
    }

    Texture* MeshDX12::GetIndexBufferSRV() const
    {
        return gResourceManager->GetTexture(mIndexBufferSRV);
    }

    Texture* MeshDX12::GetVertexBufferSRV() const
    {
        return gResourceManager->GetTexture(mVertexBufferSRV);
    }
}

