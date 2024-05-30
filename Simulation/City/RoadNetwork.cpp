#pragma once
#include "pch.h"
#include "RoadNetwork.h"
#include <xmmintrin.h>

namespace nv::sim::city
{
    using namespace math;

    float GetDistance(const float2& start, const float2& end)
    {
        return sqrtf((end.x - start.x) * (end.x - start.x) + (end.y - start.y) * (end.y - start.y));
    }

    void GetDistanceSIMD(const float2* start, const float2* end, size_t count, float* outDists)
    {
        // SIMD implementation of GetDistance
        for (size_t i = 0; i < count / 4; ++i)
        {
            __m128 startPtr = _mm_load_ps((float*)&start[i]);
            __m128 endPtr = _mm_load_ps((float*)&end[i+4]);
            __m128 outPtr;

            __m128 x = _mm_sub_ps(endPtr, startPtr);
            __m128 y = _mm_sub_ps(endPtr, startPtr);
            __m128 x2 = _mm_mul_ps(x, x);
            __m128 y2 = _mm_mul_ps(y, y);
            __m128 sum = _mm_add_ps(x2, y2);
            outPtr = _mm_sqrt_ps(sum);
            _mm_store1_ps(&outDists[i], outPtr);
        }
    }

    void RoadGraph::AddEdge(const RoadEdge& edge)
    {
        //mGraph[edge.mStart].push_back(edge.mEnd);
        //mGraph[edge.mEnd].push_back(edge.mStart);
    }

    void RoadGraph::RemoveEdge(const RoadEdge& edge)
    {
        //const auto& start = mGraph.find(edge.mStart);
        //if (start != mGraph.end())
        //{
        //    auto& edges = start->second;
        //    std::remove(edges.begin(), edges.end(), edge.mEnd);
        //}

        //const auto& end = mGraph.find(edge.mEnd);
        //if (end != mGraph.end())
        //{
        //    auto& edges = end->second;
        //    std::remove(edges.begin(), edges.end(), edge.mStart);
        //}
    }

    std::vector<float2> RoadGraph::GetPath(const float2& start, const float2& end)
    {
        // A* algorithm to find path between start and end
        std::vector<float2> path;

        // Implement A* algorithm here


        return path;
    }
}