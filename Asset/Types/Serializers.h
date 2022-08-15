#pragma once

#include <Renderer/Mesh.h>
#include <Asset.h>

namespace cereal
{
    using namespace nv::math;
    using namespace nv::graphics;
    using namespace nv::asset;

    template<class Archive>
    void serialize(Archive& archive, float3 & m)
    {
        archive(m.x);
        archive(m.y);
        archive(m.z);
    }

    template<class Archive>
    void serialize(Archive& archive, float2 & m)
    {
        archive(m.x);
        archive(m.y);
    }

    template<class Archive>
    void serialize(Archive& archive, MeshEntry & m)
    {
        archive(m.mBaseIndex);
        archive(m.mBaseVertex);
        archive(m.mNumIndices);
    }

    template<class Archive>
    void serialize(Archive& archive, Vertex & v)
    {
        archive(v.mPosition);
        archive(v.mNormal);
        archive(v.mTangent);
        archive(v.mUV);
    }

    template<class Archive>
    void serialize(Archive& archive, MeshDesc & m)
    {
        archive(m.mMeshEntries);
        archive(m.mVertices);
        archive(m.mIndices);
    }

    template<class Archive>
    void serialize(Archive& archive, Header& h)
    {
        archive(h.mAssetId.mId);
        archive(h.mOffset);
        archive(h.mSizeBytes);
    }
}