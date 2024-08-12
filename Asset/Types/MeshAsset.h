#pragma once

#include <Renderer/Mesh.h>
#include <Animation/Animation.h>
#include <Asset.h>
#include <ostream>
#include <Components/Material.h>
#include <span>

struct aiScene;

namespace nv::asset
{
    class MeshAsset 
    {
    private:
        using MeshDesc = graphics::MeshDesc;
        using AnimationStore = graphics::animation::AnimationStore;
        using MeshAnimNodeData = graphics::animation::MeshAnimNodeData;
        using Material = graphics::Material;

        struct TextureData
        {
            AssetID mTextureId;
            std::vector<uint8_t> mTextureData;

            template<typename Archive>
            void serialize(Archive& archive)
            {
                archive(mTextureId);
                archive(mTextureData);
            }
        };

        struct MatPair
        {
            std::string mName;
            Material    mMat = {};
            std::vector<TextureData> mTextures = {};

            template<typename Archive>
            void serialize(Archive& archive)
            {
                archive(mName);
                archive(mMat);
                archive(mTextures);
            }
        };

    public:
        MeshAsset(const std::string& filePath) :
            mFilePath(filePath) {}

        MeshAsset() :
            mFilePath("") {}

        void Deserialize(const AssetData& data);
        void Export(const AssetData& data, std::ostream& ostream);
        void Export(const char* pFilename, std::ostream& ostream);
        void ExportScene(const aiScene* pScene, std::ostream& ostream);
        void Register(Handle<graphics::Mesh> handle);

        const MeshDesc& GetData() const { return mData; }
        const std::vector<MatPair>& GetMaterials() const { return mMaterials; }
        ~MeshAsset();

    private:
        MeshDesc                mData = {};
        AnimationStore          mAnimStore = {};
        MeshAnimNodeData        mAnimNodeData = {};
        std::vector<MatPair>    mMaterials = {};
        std::string             mFilePath;
    };
}