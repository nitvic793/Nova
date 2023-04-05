#include <pch.h>

#include <BVH/BVH.h>
#include <Renderer/Mesh.h>
#include <Debug/Profiler.h>

namespace nv::graphics::bvh
{
    void GetTriangulatedMesh(const Mesh* mesh, TriangulatedMesh& outMesh)
    {
        using namespace math;

        outMesh.Tris.resize(mesh->GetIndexCount() / 3);
        outMesh.TriExs.resize(mesh->GetIndexCount() / 3);

        const auto& desc = mesh->GetDesc();
        for (uint32_t i = 0; i < desc.mIndices.size(); i += 3)
        {
            const uint32_t idx0 = desc.mIndices[i];
            const uint32_t idx1 = desc.mIndices[i + 1];
            const uint32_t idx2 = desc.mIndices[i + 2];

            Tri tri = 
            { 
                .Vertex0 = desc.mVertices[idx0].mPosition,
                .Vertex1 = desc.mVertices[idx1].mPosition,
                .Vertex2 = desc.mVertices[idx2].mPosition
            };

            // Calculate Centroid
            math::Vector v0 = Load(tri.Vertex0);
            math::Vector v1 = Load(tri.Vertex1);
            math::Vector v2 = Load(tri.Vertex2);

            math::Vector centroid = (v0 + v1 + v2) * 0.3333f;
            Store(centroid, tri.Centroid);

            TriEx triEx = 
            {
                .uv0 = desc.mVertices[idx0].mUV,
                .uv1 = desc.mVertices[idx1].mUV,
                .uv2 = desc.mVertices[idx2].mUV,

                .N0 = desc.mVertices[idx0].mNormal,
                .N1 = desc.mVertices[idx1].mNormal,
                .N2 = desc.mVertices[idx2].mNormal
            };

            outMesh.Tris[i / 3] = tri;
            outMesh.TriExs[i / 3] = triEx;
        }
    }

    void UpdateNodeBounds(uint nodeIdx, BVHData& outBvh, TriangulatedMesh& triMesh)
    {
        BVHNode& node = outBvh.mBvhNodes[nodeIdx];
        
        node.AABBMin = float3(DIST_MAX, DIST_MAX, DIST_MAX);
        node.AABBMax = float3(-DIST_MAX, -DIST_MAX, -DIST_MAX);
        for (uint first = node.LeftFirst, i = 0; i < node.TriCount; i++)
        {
            uint leafTriIdx = outBvh.TriIdx[first + i];
            Tri& leafTri = triMesh.Tris[leafTriIdx];
            node.AABBMin = Float3Min(node.AABBMin, leafTri.Vertex0);
            node.AABBMin = Float3Min(node.AABBMin, leafTri.Vertex1);
            node.AABBMin = Float3Min(node.AABBMin, leafTri.Vertex2);
            node.AABBMax = Float3Max(node.AABBMax, leafTri.Vertex0);
            node.AABBMax = Float3Max(node.AABBMax, leafTri.Vertex1);
            node.AABBMax = Float3Max(node.AABBMax, leafTri.Vertex2);
        }
    }

    float CalculateNodeCost(BVHNode& node)
    {
        using Vector = math::Vector;

        auto max = Load(node.AABBMax);
        auto min = Load(node.AABBMin);
        Vector extent = max - min; // extent of parent

        float3 e;
        Store(extent, e);

        float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
        float cost = node.TriCount * surfaceArea;

        return cost;
    }

    struct Bin
    {
        AABB Bounds;
        int TriCount = 0;
    };

    constexpr float GetAxis(const float3& f, int axis)
    {
        switch (axis)
        {
        case 0: return f.x;
        case 1: return f.y;
        case 2: return f.z;
        }

        assert(false);
        return 0.f;
    };

    float FindBestSplitPlane(BVHNode& node, BVHData& bvhData, TriangulatedMesh& triMesh, int& outAxis, float& splitPos)
    {
        constexpr uint32_t BINS = 8;
        int bestAxis = -1;
        float bestPos = 0;
        float bestCost = DIST_MAX;

        auto& tris = triMesh.Tris;
        auto& triExs = triMesh.TriExs;
        auto& triIdx = bvhData.TriIdx;

        constexpr float PLANE_INTERVALS = 8;

        for (int axis = 0; axis < 3; axis++)
        {
            float boundsMin = DIST_MAX;
            float boundsMax = -DIST_MAX;

            for (uint i = 0; i < node.TriCount; ++i)
            {
                Tri& triangle = tris[triIdx[node.LeftFirst + i]];
                boundsMin = std::min(boundsMin, GetAxis(triangle.Centroid, axis));
                boundsMax = std::max(boundsMax, GetAxis(triangle.Centroid, axis));
            }

            if (boundsMin == boundsMax)
                continue;

            Bin bins[BINS];
            float scale = BINS / (boundsMax - boundsMin);
            for (uint i = 0; i < node.TriCount; ++i)
            {
                Tri& triangle = tris[triIdx[node.LeftFirst + i]];
                int binIdx = std::min((int)BINS - 1, (int)((GetAxis(triangle.Centroid, axis) - boundsMin) * scale));
                bins[binIdx].TriCount++;
                bins[binIdx].Bounds.Grow(triangle.Vertex0);
                bins[binIdx].Bounds.Grow(triangle.Vertex1);
                bins[binIdx].Bounds.Grow(triangle.Vertex2);
            }

            float leftArea[BINS - 1];
            float rightArea[BINS - 1];
            int leftCount[BINS - 1];
            int rightCount[BINS - 1];

            AABB leftBox;
            AABB rightBox;
            int leftSum = 0;
            int rightSum = 0;

            for (int i = 0; i < BINS - 1; ++i)
            {
                leftSum += bins[i].TriCount;
                leftCount[i] = leftSum;
                leftBox.Grow(bins[i].Bounds);
                leftArea[i] = leftBox.Area();
                rightSum += bins[BINS - 1 - i].TriCount;
                rightCount[BINS - 2 - i] = rightSum;
                rightBox.Grow(bins[BINS - 1 - i].Bounds);
                rightArea[BINS - 2 - i] = rightBox.Area();
            }

            scale = (boundsMax - boundsMin) / BINS;

            for (uint i = 0; i < BINS - 1; ++i)
            {
                float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
                if (planeCost < bestCost)
                {
                    bestCost = planeCost;
                    splitPos = boundsMin + scale * (i + 1);
                    outAxis = axis;
                }
            }
        }

        return bestCost;
    }

    void Subdivide(uint nodeIdx, BVHData& bvhData, TriangulatedMesh& triMesh)
    {
        auto& tris = triMesh.Tris;
        auto& triExs = triMesh.TriExs;
        auto& triIdx = bvhData.TriIdx;

        BVHNode& node = bvhData.mBvhNodes[nodeIdx];
        auto& nodesUsed = bvhData.nodesUsed;

        // determine split axis using SAH
        int axis;
        float splitPos;
        float splitCost = FindBestSplitPlane(node, bvhData, triMesh, axis, splitPos);

        float noSplitCost = CalculateNodeCost(node);
        if (splitCost >= noSplitCost)
            return;

        int i = node.LeftFirst;
        int j = i + node.TriCount - 1;
        while (i <= j)
        {
            if (GetAxis(tris[triIdx[i]].Centroid, axis) < splitPos)
                i++;
            else
                std::swap(triIdx[i], triIdx[j--]);
        }

        int leftCount = i - node.LeftFirst;
        if (leftCount == 0 || leftCount == node.TriCount) return;

        int leftChildIdx = nodesUsed++;
        int rightChildIdx = nodesUsed++;
        bvhData.mBvhNodes[leftChildIdx].LeftFirst = node.LeftFirst;
        bvhData.mBvhNodes[leftChildIdx].TriCount = leftCount;
        bvhData.mBvhNodes[rightChildIdx].LeftFirst = i;
        bvhData.mBvhNodes[rightChildIdx].TriCount = node.TriCount - leftCount;
        node.LeftFirst = leftChildIdx;
        node.TriCount = 0;
        UpdateNodeBounds(leftChildIdx, bvhData, triMesh);
        UpdateNodeBounds(rightChildIdx, bvhData, triMesh);

        Subdivide(leftChildIdx, bvhData, triMesh);
        Subdivide(rightChildIdx, bvhData, triMesh);
    }

    void BuildBVH(const Mesh* mesh, BVHData& outBvh)
    {
        TriangulatedMesh triMesh;
        GetTriangulatedMesh(mesh, triMesh);

        const auto triCount = triMesh.Tris.size();
        outBvh.mBvhNodes.resize(triMesh.Tris.size() * 2);
        outBvh.TriIdx.resize(triMesh.Tris.size());

        for (uint32_t i = 0; i < triCount; i++)
            outBvh.TriIdx[i] = i;

        auto& nodesUsed = outBvh.nodesUsed;
        auto& root = *outBvh.mBvhNodes.data();

        root.LeftFirst = 0;
        root.TriCount = (uint32_t)triCount;
        uint32_t rootNodeIdx = 0;
        UpdateNodeBounds(rootNodeIdx, outBvh, triMesh);

        {
            NV_EVENT("Renderer/BVH/Build");
            Subdivide(rootNodeIdx, outBvh, triMesh);
        }
    }
}