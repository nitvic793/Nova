#pragma once

#include <Lib/Handle.h>
#include <functional>

namespace nv::jobs
{
    class Job;
}

namespace nv::asset
{
    constexpr const char RAW_ASSET_PATH[] = "../../Data/";

    class Asset;
    struct AssetID;
    enum AssetType : uint32_t;

    using AssetLoadCallback = std::function<void(Asset*)>;

    class IAssetManager
    {
    public:
        virtual void Init(const char* assetPath) = 0;
        virtual Asset* GetAsset(AssetID id) const = 0;
        virtual Asset* GetAsset(Handle<Asset> asset) const = 0;
        virtual Handle<Asset> LoadAsset(AssetID id, AssetLoadCallback callback = nullptr, bool wait = false) = 0;
        virtual void UnloadAsset(Handle<Asset> asset) = 0;
        virtual void UnloadAsset(AssetID asset) = 0;
        virtual Handle<jobs::Job> ExportAssets(const char* exportPath, bool& result, bool bForce = false) = 0;

        virtual void GetAssetsOfType(const AssetType& type, std::vector<Asset*>& assets) = 0;

        virtual void Reload(const char* file) = 0;

        virtual void Tick() = 0;

        virtual ~IAssetManager() {}
    };

    extern IAssetManager* gpAssetManager;

    void            InitAssetManager(const char* assetPath);
    void            DestroyAssetManager();
    IAssetManager*  GetAssetManager();
}