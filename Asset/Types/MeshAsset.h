#pragma once

#include <Renderer/Mesh.h>
#include <Animation/Animation.h>
#include <Asset.h>
#include <ostream>

struct aiScene;

namespace nv::asset
{
    class MeshAsset 
    {
        using MeshDesc = graphics::MeshDesc;
        using AnimationStore = graphics::animation::AnimationStore;
        using MeshAnimNodeData = graphics::animation::MeshAnimNodeData;

    public:
        void Deserialize(const AssetData& data);
        void Export(const AssetData& data, std::ostream& ostream);
        void Export(const char* pFilename, std::ostream& ostream);
        void ExportScene(const aiScene* pScene, std::ostream& ostream);
        void Register(Handle<graphics::Mesh> handle);

        const MeshDesc& GetData() const { return mData; }
        ~MeshAsset();

    private:
        MeshDesc                mData = {};
        AnimationStore          mAnimStore;
        MeshAnimNodeData        mAnimNodeData;
    };
}