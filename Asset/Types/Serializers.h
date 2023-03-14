#pragma once

#include <Renderer/Mesh.h>
#include <Asset.h>
#include <Lib/Vector.h>

#include <cereal/cereal.hpp>

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

    template<class Archive, typename T>
    typename std::enable_if<traits::is_input_serializable<BinaryData<T>, Archive>::value
        && std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, void>::type
    serialize(Archive& archive, nv::Vector<T>& v)
    {
        archive(make_size_tag(static_cast<size_type>(v.Size())));
        archive(binary_data(v.Data(), static_cast<std::size_t>(v.Size()) * sizeof(T)));
    }

    template<class Archive, typename T>
    typename std::enable_if<traits::is_input_serializable<BinaryData<T>, Archive>::value
        && std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, void>::type
    serialize(Archive& archive, const nv::Vector<T>& v)
    {
        archive(make_size_tag(static_cast<size_type>(v.Size())));
        archive(binary_data(v.Data(), static_cast<std::size_t>(v.Size()) * sizeof(T)));
    }

    template<class Archive, typename T>
    typename std::enable_if<(!traits::is_output_serializable<BinaryData<T>, Archive>::value
        || !std::is_arithmetic<T>::value) && !std::is_same<T, bool>::value, void>::type
    serialize(Archive& archive, const nv::Vector<T>& v)
    {
        archive(make_size_tag(static_cast<size_type>(v.Size())));
        for (auto&& val : v)
            archive(val);
    }

    template<class Archive, typename T>
    typename std::enable_if<(!traits::is_output_serializable<BinaryData<T>, Archive>::value
        || !std::is_arithmetic<T>::value) && !std::is_same<T, bool>::value, void>::type
    serialize(Archive& archive, nv::Vector<T>& v)
    {
        archive(make_size_tag(static_cast<size_type>(v.Size())));
        for (auto&& val : v)
            archive(val);
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
    void serialize(Archive& archive, VertexPos& v)
    {
        archive(v.mPosition);
    }

    template<class Archive>
    void serialize(Archive& archive, VertexEx& v)
    {
        archive(v.mNormal);
        archive(v.mTangent);
        archive(v.mUV);
    }

    template<class Archive>
    void serialize(Archive& archive, MeshDesc & m)
    {
        archive(m.mMeshEntries);
        archive(m.mVertices);
        archive(m.mVertexPosList);
        archive(m.mVertexExList);
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