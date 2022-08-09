#include "pch.h"
#include "AssetManager.h"
#include <Asset.h>
#include <Lib/Map.h>

namespace nv::asset
{ 
    IAssetManager* gpAssetManager = nullptr;

    class AssetManager : public IAssetManager
    {
    public:
        virtual void Init(const char* assetPath) override
        {
        }

        virtual Asset* GetAsset(AssetID id) const override
        {
            return nullptr;
        }

        virtual Handle<Asset> LoadAsset(AssetID id) const override
        {
            return Handle<Asset>();
        }

    private:

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
