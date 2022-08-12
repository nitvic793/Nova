#include "pch.h"
#include "MeshAsset.h"
#include <Types/Serializers.h>

#include <assimp/PostProcess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <cereal/archives/binary.hpp>

#include <IO/File.h>

using namespace nv::graphics;

namespace nv::asset
{
    void MeshAsset::Serialize(AssetData& data)
    {
        // No Need?
    }

    void MeshAsset::Deserialize(const AssetData& data)
    {
        //Import(data);
    }

    void MeshAsset::Export(const AssetData& data)
    {
        static Assimp::Importer importer;

        const aiScene* pScene = importer.ReadFileFromMemory(data.mData, data.mSize,
            aiProcess_Triangulate |
            aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);

        uint32_t numVertices = 0;
        uint32_t numIndices = 0;
        std::vector<MeshEntry> meshEntries(pScene->mNumMeshes);
        for (uint32_t i = 0; i < meshEntries.size(); i++)
        {
            meshEntries[i].mNumIndices = pScene->mMeshes[i]->mNumFaces * 3;
            meshEntries[i].mBaseVertex = numVertices;
            meshEntries[i].mBaseIndex = numIndices;

            numVertices += pScene->mMeshes[i]->mNumVertices;
            numIndices += meshEntries[i].mNumIndices;
        }
    }

    MeshAsset::~MeshAsset()
    {
    }
}
