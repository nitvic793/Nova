#pragma once

#include <Interop/ShaderInteropTypes.h>
#include <Engine/Component.h>
#include <vector>

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
        std::vector<Vertex>     mVertices;
        std::vector<uint32_t>   mIndices;
        std::vector<MeshEntry>  mMeshEntries;
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

    protected:
        MeshDesc mDesc;
    };
}