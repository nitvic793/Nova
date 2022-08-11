#include "pch.h"
#include "AssetManager.h"
#include <Asset.h>
#include <Lib/Map.h>
#include <Lib/Pool.h>
#include <Lib/StringHash.h>
#include <Engine/JobSystem.h>
#include <IO/Utility.h>

#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

#define NV_ASSET_DEBUG_LOADER 1

namespace nv::asset
{ 
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
        std::string outPath = path;
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

                auto path = entry.path().string();
                auto relative = GetNormalizedPath(fs::relative(path, fs::current_path()).string());

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
                    struct Payload
                    {
                        std::string mPath;
                        Asset*      mpAsset;
                        std::mutex& mMutex;
                        AssetData   mBuffer;
                        uint64_t    mId;
                    };

                    size_t size = io::GetFileSize(path.c_str());
                    Byte* pBuffer = (Byte*)Alloc(size);

                    Payload payload = { path, asset, mMutex, { size, pBuffer }, id.mId };

                    auto handle = jobs::Execute([](void* ctx)
                    {
                        auto payload = (Payload*)ctx;
                        assert(payload);
                        payload->mpAsset->SetState(STATE_LOADING); // TODO: Set state atomic for assets
                        bool result = io::ReadFile(payload->mPath.c_str(), payload->mBuffer.mData, (uint32_t)payload->mBuffer.mSize);
                        payload->mpAsset->Set({ .mId = payload->mId }, payload->mBuffer);
                        payload->mpAsset->SetState(result ? STATE_LOADED : STATE_ERROR);

                    }, &payload);

                    jobs::Wait(handle);
                }

                return it->second;
            }
#endif
            return Null<Asset>();
        }

        virtual void UnloadAsset(Handle<Asset> asset) override
        {

        }

        ~AssetManager()
        {
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
