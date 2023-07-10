#include "pch.h"

#include <Types/Serializers.h>
#include "MeshAsset.h"
#include <Animation/Animation.h>
#include <Renderer/ResourceManager.h>

#include <assimp/PostProcess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>

#include <IO/File.h>
#include <DirectXMesh.h>
#include <queue>

using namespace nv::graphics;

namespace
{
	class MathHelper
	{
	public:
		static float RandF()
		{
			return (float)(rand()) / (float)RAND_MAX;
		}

		// Returns random float in [a, b).
		static float RandF(float a, float b)
		{
			return a + RandF() * (b - a);
		}

		static float4x4 Identity4x4()
		{
			static float4x4 I(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			return I;
		}

		static float4x4 aiMatrixToXMFloat4x4Transposed(const aiMatrix4x4* aiMe)
		{
			auto offset = *aiMe;
			float4x4 output;
			Matrix mat = DirectX::XMMatrixTranspose(
				DirectX::XMMATRIX(offset.a1, offset.a2, offset.a3, offset.a4,
					offset.b1, offset.b2, offset.b3, offset.b4,
					offset.c1, offset.c2, offset.c3, offset.c4,
					offset.d1, offset.d2, offset.d3, offset.d4));
			Store(mat, output);
			return output;
		}


		static float4x4 aiMatrixToXMFloat4x4(const aiMatrix4x4* aiMe)
		{
			auto offset = *aiMe;
			float4x4 output;
			Matrix mat =
				DirectX::XMMATRIX(offset.a1, offset.a2, offset.a3, offset.a4,
					offset.b1, offset.b2, offset.b3, offset.b4,
					offset.c1, offset.c2, offset.c3, offset.c4,
					offset.d1, offset.d2, offset.d3, offset.d4);
			Store(mat, output);
			return output;
		}

		static float3x3 aiMatrixToXMFloat3x3(const aiMatrix3x3* aiMe)
		{
			float3x3 output;
			output._11 = aiMe->a1;
			output._12 = aiMe->a2;
			output._13 = aiMe->a3;

			output._21 = aiMe->b1;
			output._22 = aiMe->b2;
			output._23 = aiMe->b3;

			output._31 = aiMe->c1;
			output._32 = aiMe->c2;
			output._33 = aiMe->c3;

			return output;
		}
	};

}

namespace nv::asset
{
	using namespace animation;

	namespace utility
	{
		void LoadAnimations(const aiScene* scene, MeshAnimNodeData& animData, AnimationStore& store);
		void LoadBones(UINT index, const aiMesh* mesh, const aiScene* scene, std::vector<MeshEntry> meshEntries, std::unordered_map<std::string, uint32_t>& boneMapping, std::vector<BoneInfo>& boneInfoList, std::vector<VertexBoneData>& bones, uint32_t& numBones, uint32_t& boneIndex);
		void ProcessMesh(uint32_t index, aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	}

	void MeshAsset::Deserialize(const AssetData& data)
	{
		nv::io::MemoryStream istream((char*)data.mData, data.mSize);
		cereal::BinaryInputArchive archive(istream);

		archive(mData);
		archive(mAnimNodeData);
		archive(mAnimStore);
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
		bool bHasBones = false;

		mData.mMeshEntries.resize(numMeshes);
		for (uint32_t i = 0; i < numMeshes; i++)
		{
			constexpr uint32_t kNumPerFace = 3;
			mData.mMeshEntries[i].mNumIndices = pScene->mMeshes[i]->mNumFaces * 3;
			mData.mMeshEntries[i].mBaseVertex = numVertices;
			mData.mMeshEntries[i].mBaseIndex = numIndices;

			numVertices += pScene->mMeshes[i]->mNumVertices;
			numIndices += pScene->mMeshes[i]->mNumFaces * kNumPerFace;
			bHasBones = bHasBones || pScene->mMeshes[i]->HasBones();
		}

		mData.mIndices.reserve(numIndices);
		mData.mVertices.reserve(numVertices);
		if(bHasBones)
			mData.mBoneDesc.mBones.resize(numVertices);

		uint32_t numBones = 0;
		uint32_t boneIndex = 0;
		for (uint32_t i = 0; i < numMeshes; ++i)
		{
			utility::ProcessMesh(i, pScene->mMeshes[i], pScene, mData.mVertices, mData.mIndices);
			utility::LoadBones(i, pScene->mMeshes[i], pScene, 
				mData.mMeshEntries, mData.mBoneDesc.mBoneMapping, mData.mBoneDesc.mBoneInfoList, mData.mBoneDesc.mBones, 
				numBones, boneIndex);
		}

		AnimationStore animStore;
		MeshAnimNodeData animNodeData;
		if (pScene->HasAnimations())
		{
			utility::LoadAnimations(pScene, animNodeData, animStore);
		}

		cereal::BinaryOutputArchive archive(ostream);
		archive(mData);
		archive(animNodeData);
		archive(animStore);
	}

	void MeshAsset::Register(Handle<graphics::Mesh> handle)
	{
		gAnimManager.Register(handle, mAnimNodeData);
		gAnimManager.Register(mAnimStore);
	}

	MeshAsset::~MeshAsset()
	{
	}

	namespace utility
	{
		void LoadBones(UINT index, const aiMesh* mesh, const aiScene* scene, std::vector<MeshEntry> meshEntries, std::unordered_map<std::string, uint32_t>& boneMapping, std::vector<BoneInfo>& boneInfoList, std::vector<VertexBoneData>& bones, uint32_t& numBones, uint32_t& boneIndex)
		{
			if (mesh->HasBones())
			{
				auto globalTransform = MathHelper::aiMatrixToXMFloat4x4(&scene->mRootNode->mTransformation);
				XMFLOAT4X4 invGlobalTransform;
				XMStoreFloat4x4(&invGlobalTransform, XMMatrixInverse(nullptr, XMLoadFloat4x4(&globalTransform)));
				//uint32_t numBones = 0;
				for (uint32_t i = 0; i < mesh->mNumBones; i++)
				{
					//uint32_t boneIndex = 0;
					std::string boneName(mesh->mBones[i]->mName.data);
					if (boneMapping.find(boneName) == boneMapping.end()) //if bone not found
					{
						boneIndex = numBones;
						numBones++;
						BoneInfo bi = {};
						boneInfoList.push_back(bi);
					}
					else
					{
						boneIndex = boneMapping[boneName];
					}

					boneMapping[boneName] = boneIndex;
					boneInfoList[boneIndex].OffsetMatrix = MathHelper::aiMatrixToXMFloat4x4(&mesh->mBones[i]->mOffsetMatrix);

					for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; j++)
					{
						uint32_t vertexID = meshEntries[index].mBaseVertex + mesh->mBones[i]->mWeights[j].mVertexId;
						float weight = mesh->mBones[i]->mWeights[j].mWeight;
						bones[vertexID].AddBoneData(boneIndex, weight);
					}
				}
			}
		}

		void ProcessMesh(uint32_t index, aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
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
				}
			}
		}

		void CopyTransformChannel(aiNodeAnim* animNode, AnimationChannel& channel)
		{
			channel.NodeName = std::string(animNode->mNodeName.data);
			channel.PositionKeys.resize(animNode->mNumPositionKeys);
			channel.RotationKeys.resize(animNode->mNumRotationKeys);
			channel.ScalingKeys.resize(animNode->mNumScalingKeys);

			memcpy(&channel.PositionKeys[0], animNode->mPositionKeys, sizeof(aiVectorKey) * animNode->mNumPositionKeys);
			memcpy(&channel.RotationKeys[0], animNode->mRotationKeys, sizeof(aiVectorKey) * animNode->mNumRotationKeys);
			memcpy(&channel.ScalingKeys[0], animNode->mScalingKeys, sizeof(aiVectorKey) * animNode->mNumScalingKeys);
		}

		void LoadAnimations(const aiScene* scene, MeshAnimNodeData& animData, AnimationStore& store)
		{
			//Get global inverse
			auto globalTransform = MathHelper::aiMatrixToXMFloat4x4(&scene->mRootNode->mTransformation);
			XMStoreFloat4x4(&animData.GlobalInverseTransform, XMMatrixInverse(nullptr, XMLoadFloat4x4(&globalTransform)));

			animData.RootNode = std::string(scene->mRootNode->mName.data);
			store.Animations.resize(scene->mNumAnimations);
			std::queue<aiNode*> nodeQueue;
			nodeQueue.push(scene->mRootNode);
			XMFLOAT4X4 transform;

			//Flatten Heirarchy and map node heirarchy 
			while (!nodeQueue.empty())
			{
				auto node = nodeQueue.front();
				auto name = std::string(node->mName.data);
				auto transformation = MathHelper::aiMatrixToXMFloat4x4(&node->mTransformation);
				XMMATRIX NodeTransformation = XMMatrixTranspose(XMLoadFloat4x4(&transformation)); // TODO: Do we need to transpose?
				if (animData.NodeHeirarchy.find(name) == animData.NodeHeirarchy.end()) //If node not in heirarchy
				{
					std::vector<std::string> children;
					children.resize(node->mNumChildren);
					for (auto i = 0u; i < node->mNumChildren; ++i)
					{
						auto child = node->mChildren[i];
						auto childName = std::string(child->mName.data);
						children[i] = childName;
						nodeQueue.push(child);
					}

					XMStoreFloat4x4(&transform, NodeTransformation);
					animData.NodeTransformsMap.insert(std::pair<std::string, XMFLOAT4X4>(name, transform)); //Store node transform
					animData.NodeHeirarchy.insert(std::pair<std::string, std::vector<std::string>>(name, children));
				}

				nodeQueue.pop();
			}

			//Load animations
			auto& anims = store.Animations;
			for (auto i = 0u; i < scene->mNumAnimations; ++i)
			{
				Animation animation;

				animation.Duration = scene->mAnimations[i]->mDuration;
				animation.TicksPerSecond = scene->mAnimations[i]->mTicksPerSecond;
				animation.Channels.resize(scene->mAnimations[i]->mNumChannels);
				auto animName = std::string(scene->mAnimations[i]->mName.data);
				for (auto cIndex = 0u; cIndex < animation.Channels.size(); ++cIndex)
				{
					CopyTransformChannel(scene->mAnimations[i]->mChannels[cIndex], animation.Channels[cIndex]);
				}

				animation.AnimationName = animName;
				anims[i] = animation;
				store.AnimationIndexMap.insert(std::pair<std::string, uint32_t>(animName, i));
			}

			for (auto& anim : anims)
			{
				auto index = 0u;
				for (auto& channel : anim.Channels)
				{
					anim.NodeChannelMap.insert(std::pair<std::string, uint32_t>(channel.NodeName, index));
					index++;
				}
			}
		}
	}
}
