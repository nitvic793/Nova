#pragma once

#include <Renderer/Mesh.h>
#include <Asset.h>
#include <ostream>

namespace nv::asset
{
    class MeshAsset 
    {
        using MeshDesc = graphics::MeshDesc;

    public:
        void Deserialize(const AssetData& data);
        void Export(const AssetData& data, std::ostream& ostream);

        const MeshDesc& GetData() const { return mData; }
        ~MeshAsset();

    private:
        MeshDesc                mData = {};
    };
}