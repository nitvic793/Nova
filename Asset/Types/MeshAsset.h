#pragma once

#include <Renderer/Mesh.h>
#include <Asset.h>

namespace nv::asset
{
    class MeshAsset 
    {
    public:
        void Serialize(const AssetData& data);

    private:
        graphics::MeshDesc mData;
    };
}