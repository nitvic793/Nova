#pragma once

#include <Lib/ScopedPtr.h>
#include <Renderer/Mesh.h>
#include <Asset.h>

namespace nv::asset
{
    class MeshAsset 
    {
        using MeshDesc = graphics::MeshDesc;

    public:
        void Serialize(AssetData& data);
        void Deserialize(const AssetData& data);
        void Export(const AssetData& data);

        const MeshDesc& GetData() const { return mData; }
        ~MeshAsset();

    private:
        MeshDesc mData = {};
        
    };
}