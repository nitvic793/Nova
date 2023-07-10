#pragma once

#include <Renderer/Mesh.h>
#include <Asset.h>
#include <Lib/Vector.h>

#include <cereal/cereal.hpp>
#include <Engine/Transform.h>

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
    load(Archive& archive, nv::Vector<T>& v)
    {
        v.Clear();
        size_type size;
        archive(make_size_tag(size));
        v.Grow(size, true);
        archive(binary_data(v.Data(), static_cast<std::size_t>(v.Size()) * sizeof(T)));
    }

    template<class Archive, typename T>
    typename std::enable_if<traits::is_input_serializable<BinaryData<T>, Archive>::value
        && std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, void>::type
        load(Archive& archive, nv::Vector<T>&& v)
    {
        v.Clear();
        size_type size;
        archive(make_size_tag(size));
        v.Grow(size, true);
        archive(binary_data(v.Data(), static_cast<std::size_t>(v.Size()) * sizeof(T)));
    }

    template<class Archive, typename T>
    typename std::enable_if<traits::is_input_serializable<BinaryData<T>, Archive>::value
        && std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, void>::type
    save(Archive& archive, const nv::Vector<T>& v)
    {
        archive(make_size_tag(static_cast<size_type>(v.Size())));
        archive(binary_data(v.Data(), static_cast<std::size_t>(v.Size()) * sizeof(T)));
    }

    template<class Archive, typename T>
    typename std::enable_if<(!traits::is_output_serializable<BinaryData<T>, Archive>::value
        || !std::is_arithmetic<T>::value) && !std::is_same<T, bool>::value, void>::type
    save(Archive& archive, const nv::Vector<T>& v)
    {
        archive(make_size_tag(static_cast<size_type>(v.Size())));
        for (auto&& val : v)
            archive(val);
    }

    template<class Archive, typename T>
    typename std::enable_if<(!traits::is_output_serializable<BinaryData<T>, Archive>::value
        || !std::is_arithmetic<T>::value) && !std::is_same<T, bool>::value, void>::type
    load(Archive& archive, nv::Vector<T>& v)
    {
        v.Clear();
        size_type size;
        archive(make_size_tag(size));
        for (size_type i = 0; i < size; ++i)
        {
            T val;
            archive(val);
            v.Push(val);
        }
    }

    template<class Archive, typename T>
    void serialize(Archive& archive, nv::Handle<T>& h)
    {
        archive(h.mHandle);
    }

    template<class Archive>
    void serialize(Archive& archive, float2 & m)
    {
        archive(m.x);
        archive(m.y);
    }


    template<class Archive>
    void serialize(Archive& archive, float4& m)
    {
        archive(m.x);
        archive(m.y);
        archive(m.z);
        archive(m.w);
    }

    template<class Archive>
    void serialize(Archive& archive, float4x4& m)
    {
        archive(m._11, m._12, m._13, m._14);
        archive(m._21, m._22, m._23, m._24);
        archive(m._31, m._32, m._33, m._34);
        archive(m._41, m._42, m._43, m._44);
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
    void serialize(Archive& archive, BoneInfo& v)
    {
        archive(v.FinalTransform);
        archive(v.OffsetMatrix);
    }

    template<class Archive>
    void serialize(Archive& archive, VertexBoneData& v)
    {
        using namespace cereal;
        archive(binary_data(&v.IDs[0], static_cast<std::size_t>(MAX_BONES_PER_VERTEX) * sizeof(uint32_t)));
        archive(binary_data(&v.Weights[0], static_cast<std::size_t>(MAX_BONES_PER_VERTEX) * sizeof(float)));
    }

    template<class Archive>
    void serialize(Archive& archive, MeshDesc & m)
    {
        archive(m.mMeshEntries);
        archive(m.mVertices);
        archive(m.mIndices);
        archive(m.mBoneDesc);
    }

    template<class Archive>
    void serialize(Archive& archive, MeshBoneDesc& m)
    {
        archive(m.mBones);
        archive(m.mBoneInfoList);
        archive(m.mBoneMapping);
    }

    template<class Archive>
    void serialize(Archive& archive, Header& h)
    {
        archive(h.mAssetId.mId);
        archive(h.mOffset);
        archive(h.mSizeBytes);
    }

    template<class Archive>
    void serialize(Archive& archive, nv::Scale& s)
    {
        archive(s.mScale);
    }

    template<class Archive>
    void serialize(Archive& archive, nv::Position& s)
    {
        archive(s.mPosition);
    }

    template<class Archive>
    void serialize(Archive& archive, nv::Rotation& s)
    {
        archive(s.mRotation);
    }
}