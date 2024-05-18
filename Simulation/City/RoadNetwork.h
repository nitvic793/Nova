#pragma once


#include <stdint.h>
#include <Lib/StringHash.h>
#include <Store/Store.h>
#include <Math/Math.h>

namespace nv::sim::city
{
    template <class T>
    constexpr void HashCombine(std::size_t& s, const T& v)
    {
        std::hash<T> h;
        s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
    }

    struct RoadEdge
    {
        math::float2 mStart;
        math::float2 mEnd;
    };

    struct Float2Hash
    {
        std::size_t operator()(const math::float2& f2) const
        {
            std::size_t hash = 0;
            HashCombine(hash, f2.x);
            HashCombine(hash, f2.y);
            return hash;
        }
    };

    // Road Network
    struct RoadID : public UInt64Property {};

    class RoadGraph
    {
        using float2 = math::float2;
    public:
        void AddEdge(const RoadEdge& edge);
        void RemoveEdge(const RoadEdge& edge);
        std::vector<float2> GetPath(const float2& start, const float2& end);

    private:
        // Graph data structure for Road Network using RoadEdge which has two points
        std::unordered_map<float2, std::vector<float2>, Float2Hash> mGraph;
    };

    class RoadNetwork
    {
        using float2 = math::float2;
        using RoadStore = DataStore<RoadID, RoadEdge>;

    public:
        // Add Road
        constexpr RoadID AddRoad(const float2& start, const float2& end)
        {
            RoadStore::InstRef inst = mRoads.Emplace(&GenerateUUID);
            inst.Set<RoadEdge>({ start, end });
            return inst.Get<RoadID>();
        }

        // Remove Road
        constexpr void RemoveRoad(RoadID id)
        {
            mRoads.Erase(id);
        }

        // Get Road
        constexpr const RoadEdge& GetRoad(RoadID id)
        {
            auto edge = mRoads.Find(id);
            return edge.Get<RoadEdge>();
        }

        // Function to generate network graph using RoadStore

    private:
        DataStore<RoadID, RoadEdge> mRoads;
        RoadGraph mGraph;
    };
}