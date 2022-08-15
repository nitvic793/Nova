#pragma once

#include <Renderer/ShaderInteropTypes.h>
#include <vector>

namespace nv::graphics
{
    struct MeshEntry
    {
        uint32_t mNumIndices;
        uint32_t mBaseVertex;
        uint32_t mBaseIndex;
    };

    struct MeshDesc
    {
        std::vector<Vertex>     mVertices;
        std::vector<uint32_t>   mIndices;
        std::vector<MeshEntry>  mMeshEntries;
    };

    class Mesh
    {

    };
}