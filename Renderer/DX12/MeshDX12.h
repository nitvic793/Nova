#pragma once

#include <Lib/Handle.h>
#include <Renderer/Mesh.h>

struct ID3D12Resource;
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;

namespace nv::graphics
{
    class GPUResource;

    class MeshDX12 : public Mesh
    {
    public:

    private:
        Handle<GPUResource> mVertexBuffer;
        Handle<GPUResource> mIndexBuffer;
    };
}