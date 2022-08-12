#pragma once

#include <Renderer/Mesh.h>

namespace nv::math
{
    template<class Archive>
    void serialize(Archive& archive, float3 const& m)
    {
        archive(m.x);
        archive(m.y);
        archive(m.z);
    }

    template<class Archive>
    void serialize(Archive& archive, float2 const& m)
    {
        archive(m.x);
        archive(m.y);
    }
}

namespace nv::graphics
{
    template<class Archive>
    void serialize(Archive& archive, MeshEntry const& m)
    {
        archive(m.mBaseIndex);
        archive(m.mBaseVertex);
        archive(m.mNumIndices);
    }

    template<class Archive>
    void serialize(Archive& archive, Vertex const& v)
    {
        archive(v.mPosition);
        archive(v.mNormal);
        archive(v.mTangent);
        archive(v.mUV);
    }

    template<class Archive>
    void save(Archive& archive, MeshDesc const& m)
    {
        archive(m.mIndexCount);
        archive(m.mVertexCount);
        archive(m.mMeshEntryCount);

        for (uint32_t i = 0; i < m.mMeshEntryCount; ++i)
        {
            archive(m.mpMeshEntries[i]);
        }

        for (uint32_t i = 0; i < m.mVertexCount; ++i)
        {
            archive(m.mpVertices[i]);
        }

        for (uint32_t i = 0; i < m.mIndexCount; ++i)
        {
            archive(m.mpIndices[i]);
        }
    }

    template<class Archive>
    void load(Archive& archive, MeshDesc& m)
    {
        archive(m.mIndexCount);
        archive(m.mVertexCount);
        archive(m.mMeshEntryCount);

        for (uint32_t i = 0; i < m.mMeshEntryCount; ++i)
        {
            archive(m.mpMeshEntries[i]);
        }

        for (uint32_t i = 0; i < m.mVertexCount; ++i)
        {
            archive(m.mpVertices[i]);
        }

        for (uint32_t i = 0; i < m.mIndexCount; ++i)
        {
            archive(m.mpIndices[i]);
        }
    }
}