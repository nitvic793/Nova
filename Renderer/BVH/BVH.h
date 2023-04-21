#pragma once

#include <Math/Math.h>
#include <Interop/ShaderInteropTypes.h>

/// BVH Reference Code: https://jacco.ompf2.com/2022/06/03/how-to-build-a-bvh-part-9a-to-the-gpu/

namespace nv::graphics
{
    class Mesh;
}

namespace nv::graphics::bvh
{
    constexpr float DIST_MAX = 1e30f;
    using namespace math;

    struct BVHNode
    {
        using float3 = math::float3;
        using Vector = math::Vector;

        float3      AABBMin;
        uint32_t    LeftFirst;
        float3      AABBMax;
        uint32_t    TriCount;
    };

    inline float3 Float3Min(const float3& a, const float3& b) noexcept
    {
        using Vector = math::Vector;

        Vector va = Load(a);
        Vector vb = Load(b);
        Vector min = VectorMin(va, vb);
        
        float3 result;
        Store(min, result);
        return result;
    }

    inline float3 Float3Max(const float3& a, const float3& b) noexcept
    {
        using Vector = math::Vector;

        Vector va = Load(a);
        Vector vb = Load(b);
        Vector max = VectorMax(va, vb);

        float3 result;
        Store(max, result);
        return result;
    }

    struct AABB
    {
        using float3 = math::float3;
        using Vector = math::Vector;

        float3 bmin = float3(DIST_MAX, DIST_MAX, DIST_MAX);
        float3 bmax = float3(-DIST_MAX, -DIST_MAX, -DIST_MAX);

        void Grow(const float3& p)
        {
            using namespace math;

            auto vp = Load(p);
            auto min = Load(bmin);
            auto max = Load(bmax);
            min = VectorMin(min, vp);
            max = VectorMax(max, vp);
            Store(min, bmin);
            Store(max, bmax);
        }

        void Grow(const AABB& bb)
        {
            using namespace math;

            auto vbbMin = Load(bb.bmin);
            auto vbbMax = Load(bb.bmax);
            auto min = Load(bmin);
            auto max = Load(bmax);
            min = VectorMin(min, vbbMin);
            max = VectorMax(max, vbbMax);
            Store(min, bmin);
            Store(max, bmax);
        }

        float Area()
        {
            Vector max = math::Load(bmax);
            Vector min = math::Load(bmin);
            Vector result = max - min; // box extent
            float3 e;
            math::Store(result, e);
            return e.x * e.y + e.y * e.z + e.z * e.x;
        }

        graphics::AABB GfxAABB() const
        {
            graphics::AABB aabb;
            aabb.bmax = bmax;
            aabb.bmin = bmin;
            return aabb;
        }
    };

    struct TriangulatedMesh;

    struct BVHData
    {
        uint32_t* triIdx = nullptr;
        std::vector<uint32_t> TriIdx;
        uint32_t  nodesUsed = 2;
        TriangulatedMesh* mesh = nullptr;
        std::vector<BVHNode> mBvhNodes;
    };

    struct BVHInstance
    {
        BVHData* bvh = 0;
        math::float4x4 transform;
        math::float4x4 invTransform; // inverse transform
        uint32_t idx;
        AABB bounds; // in world space
    };

    //struct Tri
    //{
    //    using float3 = math::float3;

    //    float3 Vertex0;
    //    float3 Vertex1;
    //    float3 Vertex2;
    //    float3 Centroid;
    //};

    //struct TriEx
    //{
    //    using float3 = math::float3;
    //    using float2 = math::float2;

    //    float2 uv0;
    //    float2 uv1;
    //    float2 uv2;
    //    float3 N0;
    //    float3 N1;
    //    float3 N2;
    //    float dummy;
    //};

    struct TriangulatedMesh
    {
        using float3 = math::float3;

        std::vector<graphics::Tri>    Tris;
        std::vector<graphics::TriEx>  TriExs;
        //Tri*        tri;
        //TriEx*      triEx;
        //float3*     P = nullptr;
        //float3*     N = nullptr;
        int         triCount = 0;
    };

    struct TLAS
    {
        nv::Vector<TLASNode> mTlasNodes;
        uint32_t mNodesUsed = 0;
        uint32_t BlasCount  = 0;
    };

    void BuildBVH(const Mesh* mesh, BVHData& outBvh, TriangulatedMesh& triMesh);
    void BuildTLAS(Span<graphics::BVHInstance> bvhInstances, Span<graphics::AABB> aabbs, TLAS& outTlas);
}