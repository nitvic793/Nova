#pragma once

#include <Renderer/ShaderInteropTypes.h>

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
        uint32_t    mVertexCount;
        uint32_t    mIndexCount;
        uint32_t    mMeshEntryCount;

        Vertex*     mpVertices;
        uint32_t*   mpIndices;
        MeshEntry*  mpMeshEntries;
    };

    class Mesh
    {

    };
}