#include "pch.h"

#include <Types/Serializers.h>
#include "MeshAsset.h"
#include <Types/TextureAsset.h>
#include <Animation/Animation.h>
#include <Renderer/ResourceManager.h>
#include <Components/Material.h>

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
#include <Engine/Log.h>

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
		archive(mMaterials);
		archive(mAnimNodeData);
		archive(mAnimStore);
	}

	void MeshAsset::Export(const AssetData& data, std::ostream& ostream)
	{
		static Assimp::Importer importer;

		const aiScene* pScene = importer.ReadFileFromMemory(data.mData, data.mSize,
			aiProcess_Triangulate |
			aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices , "");

		if (!pScene)
		{
			log::Error("[Asset] Unable to export mesh");
			return;
		}

		ExportScene(pScene, ostream);
	}

	void MeshAsset::Export(const char* pFilename, std::ostream& ostream)
	{
		static Assimp::Importer importer;
		const aiScene* pScene = importer.ReadFile(pFilename,
			aiProcess_Triangulate |
			aiProcess_ConvertToLeftHanded | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices | aiProcess_RemoveRedundantMaterials);

		if (!pScene)
		{
			log::Error("[Asset] Unable to export '{}'", pFilename);
			return;
		}

		ExportScene(pScene, ostream);
	}

	void MeshAsset::ExportScene(const aiScene* pScene, std::ostream& ostream)
	{
		const uint32_t numMeshes = pScene->mNumMeshes;
		const uint32_t numMaterials = pScene->mNumMaterials;

		uint32_t numVertices = 0;
		uint32_t numIndices = 0;
		bool bHasBones = false;

		mData.mMeshEntries.resize(numMeshes);
		for (uint32_t i = 0; i < numMeshes; i++)
		{
			const auto pMesh = pScene->mMeshes[i];
			constexpr uint32_t kNumPerFace = 3;
			mData.mMeshEntries[i].mNumIndices = pMesh->mNumFaces * 3;
			mData.mMeshEntries[i].mBaseVertex = numVertices;
			mData.mMeshEntries[i].mBaseIndex = numIndices;

			numVertices += pMesh->mNumVertices;
			numIndices += pMesh->mNumFaces * kNumPerFace;
			bHasBones = bHasBones || pMesh->HasBones();
		}

		mData.mIndices.reserve(numIndices);
		mData.mVertices.reserve(numVertices);
		if (bHasBones)
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

		if (animStore.Animations.size() == 1)
		{
			animStore.Animations[0].AnimationName = animStore.Animations[0].AnimationName + "|" + mFilePath;
			animStore.AnimationIndexMap[animStore.Animations[0].AnimationName] = 0;
		}

		if (pScene->HasMaterials())
		{
			for (uint32_t i = 0; i < numMaterials; ++i)
			{
				aiMaterial* material = pScene->mMaterials[i];
				aiString materialName = material->GetName();
				aiReturn ret;

                std::string internalMatName = "Material/" + mFilePath + "/" + materialName.C_Str();
				
				MatPair& matPair = mMaterials.emplace_back(MatPair{ internalMatName, Material{} });
				Material& mat = matPair.mMat;

				aiColor3D diffuse = {};
				aiColor3D emissive = {};
				ret = material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
				material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);

				if (ret == aiReturn_SUCCESS)
				{
					mat.mData.mSimple = {};
					mat.mType = MATERIAL_SIMPLE;
					mat.mData.mSimple.mDiffuseColor = math::float3(diffuse.r, diffuse.g, diffuse.b);
					mat.mData.mSimple.mEmissive = math::float3(emissive.r, emissive.g, emissive.b);
				}

				aiString diffuseTexName;
				aiString normalsTexName;
				aiString roughnessTexName;
				aiString metalnessTexName;

				material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffuseTexName);
				ret = ret && diffuseTexName.length == 0 ? material->Get(AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR,0), diffuseTexName) : aiReturn_SUCCESS;

				material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normalsTexName); 
				ret = ret && normalsTexName.length == 0 ? material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMAL_CAMERA, 0), normalsTexName) : aiReturn_SUCCESS;

				material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), roughnessTexName); 
				ret = ret && roughnessTexName.length == 0 ? material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), roughnessTexName) : aiReturn_SUCCESS;

                material->Get(AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0), metalnessTexName);
				ret = ret && metalnessTexName.length == 0 ? material->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), metalnessTexName) : aiReturn_SUCCESS;

				const aiTexture* diffuseTex = pScene->GetEmbeddedTexture(diffuseTexName.C_Str());
				const aiTexture* normalsTex = pScene->GetEmbeddedTexture(normalsTexName.C_Str());
				const aiTexture* roughnessTex = pScene->GetEmbeddedTexture(roughnessTexName.C_Str());
				const aiTexture* metalnessTex = pScene->GetEmbeddedTexture(metalnessTexName.C_Str());

                const auto exportTextureToMemory = [](const aiTexture* tex, std::vector<uint8_t>& buffer)
                {
                    if (!tex)
						return;
                    
					TextureAsset texAsset;
                    std::stringstream stream;
					texAsset.Export({ tex->mWidth, (uint8_t*)tex->pcData }, stream);
					buffer.resize(stream.str().size());
                    stream.read((char*)buffer.data(), buffer.size());
                };

				const auto addTexture = [&](const aiTexture* tex)
				{
					if(!tex)
                        return INVALID_TEXTURE;

					std::string textureName = tex->mFilename.C_Str();
					std::string fileName = mFilePath + "_" + textureName.substr(textureName.find_last_of("/\\") + 1);
					const StringID hash = nv::ID(fileName.c_str());
					StringDB::Get().AddString(fileName, hash);
					AssetID texId = { ASSET_TEXTURE, hash };
					auto& texPair = matPair.mTextures.emplace_back(TextureData{ .mTextureId = texId, .mTextureData = std::vector<uint8_t>() });
					exportTextureToMemory(diffuseTex, texPair.mTextureData);

					return texId;
				};
				
				if (diffuseTex)
				{
					mat.mType = MATERIAL_PBR;
					mat.mData.mPBR = {};

					mat.mData.mPBR.mAlbedoTexture		= addTexture(diffuseTex);
                    mat.mData.mPBR.mNormalTexture		= addTexture(normalsTex);
                    mat.mData.mPBR.mRoughnessTexture	= addTexture(roughnessTex);
                    mat.mData.mPBR.mMetalnessTexture	= addTexture(metalnessTex);
				}
			}
		}

		// Export mesh data
		{
			cereal::BinaryOutputArchive archive(ostream);
			archive(mData);
			archive(mMaterials);
			archive(animNodeData);
			archive(animStore);
		}
	}

	void MeshAsset::Register(Handle<graphics::Mesh> handle)
	{
		if(!handle.IsNull())
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

		void CleanUp(std::string& name)
		{
			constexpr const char cleanUpStr[] = ":";
			auto cleanUp = name.find(cleanUpStr);
			if (cleanUp != std::string::npos)
			{
				name = name.erase(0, cleanUp + 1);
			}
		};

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
                CleanUp(name);

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
                        CleanUp(childName);

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
                    CleanUp(channel.NodeName);
					anim.NodeChannelMap.insert(std::pair<std::string, uint32_t>(channel.NodeName, index));
					index++;
				}
			}
		}
	}
}
