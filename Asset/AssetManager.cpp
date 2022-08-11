#include "pch.h"
#include "AssetManager.h"
#include <Asset.h>
#include <Lib/Map.h>
#include <Lib/Pool.h>
#include <Lib/StringHash.h>
#include <Engine/JobSystem.h>
#include <Engine/Log.h>
#include <IO/Utility.h>
#include <Types/MeshAsset.h>

#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

#define NV_ASSET_DEBUG_LOADER 1

namespace nv::asset
{
    static const char* gpDataPath = nullptr;
    IAssetManager* gpAssetManager = nullptr;

    constexpr const char MESH_PATH[] = "Mesh";
    constexpr const char TEXTURES_PATH[] = "Textures";
    constexpr const char SHADERS_PATH[] = "Shaders";

    constexpr bool StringContains(const std::string& str, const std::string& inString) 
    {
        return inString.find(str) != std::string::npos;  
    }

    constexpr AssetType GetAssetType(const char* pathString)
    {
        if (StringContains(MESH_PATH, pathString))      return ASSET_MESH;
        if (StringContains(TEXTURES_PATH, pathString))  return ASSET_TEXTURE;
        if (StringContains(SHADERS_PATH, pathString))   return ASSET_SHADER;

        return ASSET_INVALID;
    }

    constexpr std::string GetNormalizedPath(const std::string& path)
    {
        const char* replaceEmpty = "";
        std::string_view dataPath = gpDataPath;
        dataPath.remove_prefix(1);
        std::string outPath = path;
        auto pos = outPath.find(dataPath);
        if(pos != std::string::npos)
            outPath = outPath.replace(pos, dataPath.size() + 1, "");
        std::replace(outPath.begin(), outPath.end(), '\\', '/');
        return outPath;
    }

    class AssetManager : public IAssetManager
    {
    public:
        virtual void Init(const char* assetPath) override
        {
            mAssets.Init();
            fs::path path = fs::current_path().string() + assetPath;
            for (const auto& entry : fs::recursive_directory_iterator(path))
            {
                if (!fs::is_regular_file(entry.path()))
                    continue;

                const auto path = entry.path().string();
                const auto relative = GetNormalizedPath(fs::relative(path, fs::current_path()).string());

                const AssetID id = { GetAssetType(relative.c_str()), ID(relative.c_str())};
                auto handle = mAssets.Create();
                Asset* asset = mAssets.Get(handle);
                asset->Set(id, {});
                asset->SetState(STATE_UNLOADED);
                mAssetMap[id.mId] = handle;
#if NV_ASSET_DEBUG_LOADER
                mAssetPathMap[id.mId] = entry.path().string();
#endif
            }
        }

        virtual Asset* GetAsset(AssetID id) const override
        {
            auto it = mAssetMap.find(id);
            if (it != mAssetMap.end())
                return mAssets.Get(it->second);

            return nullptr;
        }

        virtual Asset* GetAsset(Handle<Asset> asset) const override
        {
            return mAssets.Get(asset);
        }

        virtual Handle<Asset> LoadAsset(AssetID id) override
        {
#if NV_ASSET_DEBUG_LOADER
            auto it = mAssetMap.find(id.mId);
            if (it != mAssetMap.end())
            {
                Asset* asset = mAssets.Get(it->second);
                if(!asset)
                    return Null<Asset>();

                if (asset->GetState() != STATE_LOADED && asset->GetState() != STATE_ERROR)
                {
                    auto path = mAssetPathMap[id.mId];
                    size_t size = io::GetFileSize(path.c_str());
                    Byte* pBuffer = (Byte*)Alloc(size);

                    auto handle = jobs::Execute([=](void* ctx)
                    {
                        asset->SetState(STATE_LOADING); // TODO: Set state atomic for assets
                        AssetData data = { size, pBuffer };
                        bool result = io::ReadFile(path.c_str(), data.mData, (uint32_t)size);
                        asset->Set(id, data);
                        asset->SetState(result ? STATE_LOADED : STATE_ERROR);

#if _DEBUG
                        if (result)
                            log::Info("[Asset] Load {}: OK", path.c_str());
                        else
                            log::Error("[Asset] Load {}: ERROR", path.c_str());
#endif
                        MeshAsset mesh;
                        asset->SerializeTo(mesh);
                        asset->SerializeTo<MeshAsset>();
                    });
                }

                return it->second;
            }
#endif
            return Null<Asset>();
        }

        virtual void UnloadAsset(Handle<Asset> asset) override
        {
            Asset* pAsset = mAssets.Get(asset);
            if (pAsset)
            {
                if (pAsset->GetState() == STATE_LOADED)
                {
                    Free(pAsset->GetData());
                    pAsset->SetData({ 0, nullptr });
                    return;
                }
            }

            assert(false);
            const auto& assetPath = mAssetPathMap[pAsset->GetID()];
            log::Error("[Asset] Error unloading asset {}", assetPath.c_str());
        }

        void CleanUp()
        {
            for (auto& item : mAssetMap)
            {
                Asset* asset = mAssets.Get(item.second);
                if (asset)
                    Free(asset->GetData());
            }
        }

        ~AssetManager()
        {
            CleanUp();
            mAssets.Destroy();
        }

    protected:
        Pool<Asset>                         mAssets;
        HashMap<uint64_t, Handle<Asset>>    mAssetMap;
#if NV_ASSET_DEBUG_LOADER
        HashMap<uint64_t, std::string>      mAssetPathMap;
#endif
        std::mutex                          mMutex;
    };

    void InitAssetManager(const char* assetPath)
    {
        gpAssetManager = Alloc<AssetManager>();
        gpDataPath = assetPath;
        gpAssetManager->Init(assetPath);
    }

    void DestroyAssetManager()
    {
        Free<AssetManager>((AssetManager*)gpAssetManager);
    }

    IAssetManager* GetAssetManager()
    {
        return gpAssetManager;
    }
}
