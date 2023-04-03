#include "pch.h"

#include <Types/Serializers.h>
#include "MeshAsset.h"

#include <assimp/PostProcess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include <IO/File.h>
#include <DirectXMesh.h>

using namespace nv::graphics;

namespace nv::asset
{
    namespace utility
    {
		void ProcessMesh(uint32_t index, aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<VertexPos>& verticesPos, std::vector<VertexEx>& verticesEx)
		{
			for (uint32_t i = 0; i < mesh->mNumVertices; i++)
			{
				Vertex vertex;

				vertex.mPosition.x = mesh->mVertices[i].x;
				vertex.mPosition.y = mesh->mVertices[i].y;
				vertex.mPosition.z = mesh->mVertices[i].z;

				if (mesh->HasNormals())
					vertex.mNormal = float3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

				if (mesh->mTextureCoords[0])
				{
					vertex.mUV.x = (float)mesh->mTextureCoords[0][i].x;
					vertex.mUV.y = (float)mesh->mTextureCoords[0][i].y;
				}

				vertices.push_back(vertex);

				VertexPos vertPos = { .mPosition = vertex.mPosition };
				VertexEx vertEx = { .mUV = vertex.mUV, .mNormal = vertex.mNormal, .mTangent = vertex.mTangent };

				verticesPos.push_back(vertPos);
				verticesEx.push_back(vertEx);
			}

			for (uint32_t i = 0; i < mesh->mNumFaces; i++)
			{
				aiFace face = mesh->mFaces[i];
				
				for (uint32_t j = 0; j < face.mNumIndices; j++)
					indices.push_back(face.mIndices[j]);
			}

			if (!mesh->HasNormals() || !mesh->HasTangentsAndBitangents())
			{
				std::vector<float3> pos;
				std::vector<float3> normals;
				std::vector<float3> tangents;
				std::vector<float2> uv;
				for (auto& v : vertices)
				{
					pos.push_back(v.mPosition);
					normals.push_back(v.mNormal);
					tangents.push_back(v.mTangent);
					uv.push_back(v.mUV);
				}

				if (!mesh->HasNormals())
					DirectX::ComputeNormals(indices.data(), mesh->mNumFaces, pos.data(), pos.size(), DirectX::CNORM_DEFAULT, normals.data());

				if (!mesh->HasTangentsAndBitangents())
					DirectX::ComputeTangentFrame(indices.data(), mesh->mNumFaces, pos.data(), normals.data(), uv.data(), pos.size(), tangents.data(), nullptr);

				for (int i = 0; i < vertices.size(); ++i)
				{
					vertices[i].mNormal = normals[i];
					vertices[i].mTangent = tangents[i];

                    verticesEx[i].mNormal = normals[i];
                    verticesEx[i].mTangent = tangents[i];
				}
			}
		}
    }

    void MeshAsset::Deserialize(const AssetData& data)
    {
		nv::io::MemoryStream istream((char*)data.mData, data.mSize);
		cereal::BinaryInputArchive archive(istream);
		archive(mData);
    }

    void MeshAsset::Export(const AssetData& data, std::ostream& ostream)
    {
        static Assimp::Importer importer;

        const aiScene* pScene = importer.ReadFileFromMemory(data.mData, data.mSize,
            aiProcess_Triangulate |
            aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices);

        uint32_t numMeshes = pScene->mNumMeshes;
        uint32_t numVertices = 0;
        uint32_t numIndices = 0;

		mData.mMeshEntries.resize(numMeshes);
        for (uint32_t i = 0; i < numMeshes; i++)
        {
            constexpr uint32_t kNumPerFace = 3;
			mData.mMeshEntries[i].mNumIndices = pScene->mMeshes[i]->mNumFaces * 3;
			mData.mMeshEntries[i].mBaseVertex = numVertices;
			mData.mMeshEntries[i].mBaseIndex = numIndices;

            numVertices += pScene->mMeshes[i]->mNumVertices;
            numIndices += pScene->mMeshes[i]->mNumFaces * kNumPerFace;
        }

		mData.mIndices.reserve(numIndices);
		mData.mVertices.reserve(numVertices);
        mData.mVertexPosList.reserve(numVertices);
        mData.mVertexExList.reserve(numVertices);

		for (uint32_t i = 0; i < numMeshes; ++i)
		{
			utility::ProcessMesh(i, pScene->mMeshes[i], pScene, mData.mVertices, mData.mIndices, mData.mVertexPosList, mData.mVertexExList);
		}

		cereal::BinaryOutputArchive archive(ostream);
		archive(mData);
    }

    MeshAsset::~MeshAsset()
    {
    }
}
