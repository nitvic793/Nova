#pragma once

#include <Lib/ScopedPtr.h>
#include <Renderer/Mesh.h>
#include <Asset.h>

namespace nv::asset
{
    class MeshAsset 
    {
    public:
        void Serialize(AssetData& data);
        void Deserialize(const AssetData& data);
        void Import(const AssetData& data);

    private:
        graphics::MeshDesc mData;
        
    };
}