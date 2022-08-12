#include "pch.h"
#include "MeshAsset.h"

#include <assimp/PostProcess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

using namespace nv::graphics;

namespace nv::asset
{ 
    void MeshAsset::Serialize(AssetData& data)
    {

    }

    void MeshAsset::Deserialize(const AssetData& data)
    {
        Import(data);
    }

    void MeshAsset::Import(const AssetData& data)
    {
        static Assimp::Importer importer;

        const aiScene* pScene = importer.ReadFileFromMemory(data.mData, data.mSize,
            aiProcess_Triangulate |
            aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);
    }
}
