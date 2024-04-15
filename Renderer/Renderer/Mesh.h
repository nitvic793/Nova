#pragma once

#include <Interop/ShaderInteropTypes.h>
#include <Engine/Component.h>
#include <vector>
#include <BVH/BVH.h>
#include <Math/Collision.h>

namespace nv::graphics
{
    constexpr size_t MAX_BONES_PER_VERTEX = 4;

    struct MeshEntry
    {
        uint32_t mNumIndices;
        uint32_t mBaseVertex;
        uint32_t mBaseIndex;
    };

    struct BoneInfo
    {
        float4x4 OffsetMatrix;
        float4x4 FinalTransform;
    };

    struct VertexBoneData
    {
        uint32_t	IDs[MAX_BONES_PER_VERTEX];
        float		Weights[MAX_BONES_PER_VERTEX];

        constexpr void AddBoneData(uint32_t boneID, float weight)
        {
            for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; i++)
            {
                if (Weights[i] == 0.0)
                {
                    IDs[i] = boneID;
                    Weights[i] = weight;
                    return;
                }
            }
        }
    };

    struct MeshBoneDesc
    {
        std::unordered_map<std::string, uint32_t>	mBoneMapping;
        std::vector<BoneInfo>						mBoneInfoList;
        std::vector<VertexBoneData>					mBones;
    };

    struct MeshDesc
    {
        std::vector<Vertex>     mVertices; // TODO: Remove this and use the pos and extras list instead
        std::vector<uint32_t>   mIndices;
        std::vector<MeshEntry>  mMeshEntries;
        MeshBoneDesc            mBoneDesc;
        // TODO: 
        // Support material creation for each mesh entry
        // While drawing, sort by material and then draw mesh entry accordingly. 
    };

    class Mesh : public ecs::IComponent
    {
    public:
        Mesh() {};
        Mesh(const MeshDesc& desc) :
            mDesc(desc) {}
        virtual ~Mesh() {}
        constexpr const MeshDesc& GetDesc() const { return mDesc; }
        constexpr size_t GetVertexCount() const { return mDesc.mVertices.size(); }
        constexpr size_t GetIndexCount() const { return mDesc.mIndices.size(); }
        constexpr bool   HasBones() const { return !mDesc.mBoneDesc.mBoneInfoList.empty(); }
        constexpr const MeshBoneDesc& GetBoneData() const { return mDesc.mBoneDesc; }
        inline bvh::BVHData& GetBVH() { return mBVH; }

        void            CreateBoundingBox();
        const math::BoundingBox& GetBoundingBox() const { return mBoundingBox; }

        template<class Archive>
        void serialize(Archive& archive)
        {
        }

    protected:
        MeshDesc            mDesc;
        bvh::BVHData        mBVH;
        math::BoundingBox   mBoundingBox;
    };
}