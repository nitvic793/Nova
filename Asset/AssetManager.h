#pragma once

#include <Lib/Handle.h>

namespace nv::jobs
{
    class Job;
}

namespace nv::asset
{
    class Asset;
    struct AssetID;

    class IAssetManager
    {
    public:
        virtual void Init(const char* assetPath) = 0;
        virtual Asset* GetAsset(AssetID id) const = 0;
        virtual Asset* GetAsset(Handle<Asset> asset) const = 0;
        virtual Handle<Asset> LoadAsset(AssetID id, bool wait = false) = 0;
        virtual void UnloadAsset(Handle<Asset> asset) = 0;
        virtual Handle<jobs::Job> ExportAssets(const char* exportPath) = 0;

        virtual ~IAssetManager() {}
    };

    extern IAssetManager* gpAssetManager;

    void            InitAssetManager(const char* assetPath);
    void            DestroyAssetManager();
    IAssetManager*  GetAssetManager();
}