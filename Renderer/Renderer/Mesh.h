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
        uint32_t    mVertexCount    = 0;
        uint32_t    mIndexCount     = 0;
        uint32_t    mMeshEntryCount = 0;

        Vertex*     mpVertices      = nullptr;
        uint32_t*   mpIndices       = nullptr;
        MeshEntry*  mpMeshEntries   = nullptr;
    };

    class Mesh
    {

    };
}