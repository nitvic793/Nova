#include "pch.h"
#include "ResourceManager.h"
#include <Renderer/ResourceTracker.h>
#include <Renderer/Renderer.h>
#include <Renderer/Context.h>
#include <Components/Material.h>
#include <Renderer/Device.h>

#include <AssetManager.h>
#include <Types/TextureAsset.h>

namespace nv::graphics
{
    ResourceTracker gResourceTracker;

    ResourceManager::ResourceManager()
    {
        mMaterialPool.Init();
    }

    Handle<Texture> ResourceManager::CreateTexture(const TextureDesc& desc, ResID id)
    {
        auto handle = CreateTexture(desc);
        gResourceTracker.Track(id, handle);
        return handle;
    }

    Handle<Texture> ResourceManager::CreateTexture(asset::AssetID asset)
    {
        auto ctx = gRenderer->GetContext();
        ctx->Begin();
        auto texAsset = asset::gpAssetManager->GetAsset(asset);
        asset::TextureAsset tex = texAsset->DeserializeTo<asset::TextureAsset>(ctx);
        ctx->End();
        gRenderer->Submit(ctx);
        return gResourceManager->CreateTexture(tex.GetDesc(), asset.mHash);
    }

    Handle<Mesh> ResourceManager::CreateMesh(const MeshDesc& desc, ResID id)
    {
        auto handle = CreateMesh(desc);
        gResourceTracker.Track(id, handle);
        return handle;
    }

    Handle<Material> ResourceManager::CreateMaterial(const PBRMaterial& matDesc, ResID id)
    {
        if (gResourceTracker.ExistsMaterial(id))
            return gResourceTracker.GetMaterialHandle(id);

        const auto getOrCreateTexture = [&](asset::AssetID asset)
        {
            if (gResourceTracker.ExistsTexture(asset.mHash))
                return gResourceTracker.GetTextureHandle(matDesc.mAlbedoTexture.mHash);

            return CreateTexture(asset);
        };

        Material mat;
        mat.mType = MATERIAL_PBR;

        mat.mTextures[0] = getOrCreateTexture(matDesc.mAlbedoTexture);
        mat.mTextures[1] = getOrCreateTexture(matDesc.mNormalTexture);
        mat.mTextures[2] = getOrCreateTexture(matDesc.mRoughnessTexture);
        mat.mTextures[3] = getOrCreateTexture(matDesc.mMetalnessTexture);

        auto handle = mMaterialPool.Create(mat);
        gResourceTracker.Track(id, handle);

        return handle;
    }

    Handle<Material> ResourceManager::GetMaterialHandle(ResID id)
    {
        if (gResourceTracker.ExistsMaterial(id))
        {
            return gResourceTracker.GetMaterialHandle(id);
        }

        return Null<Material>();
    }

    Handle<Mesh> ResourceManager::GetMeshHandle(ResID id)
    {
        if (gResourceTracker.ExistsMesh(id))
        {
            return gResourceTracker.GetMeshHandle(id);
        }

        return Null<Mesh>();
    }

    Material* ResourceManager::GetMaterial(Handle<Material> handle)
    {
        return mMaterialPool.Get(handle);
    }

    Material* ResourceManager::GetMaterial(ResID id)
    {
        if (gResourceTracker.ExistsMaterial(id))
        {
            Handle<Material> handle = gResourceTracker.GetMaterialHandle(id);
            return mMaterialPool.Get(handle);
        }

        return nullptr;
    }

    Texture* ResourceManager::GetTexture(ResID id)
    {
        auto handle = gResourceTracker.GetTextureHandle(id);
        return GetTexture(handle);
    }

    ResourceManager::~ResourceManager()
    {
        mMaterialPool.Destroy();
    }
}
