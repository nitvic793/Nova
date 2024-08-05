#include "pch.h"
#include "ResourceManager.h"
#include <Renderer/ResourceTracker.h>
#include <Renderer/Renderer.h>
#include <Renderer/Context.h>
#include <Components/Material.h>
#include <Renderer/Device.h>

#include <AssetManager.h>
#include <Types/TextureAsset.h>
#include <Types/MeshAsset.h>

#include <Engine/Log.h>

namespace nv::graphics
{
    ResourceTracker gResourceTracker;

    ResourceManager::ResourceManager()
    {
        mMaterialPool.Init();
    }

    Handle<Texture> ResourceManager::CreateTexture(const TextureDesc& desc, ResID id)
    {
        assert(!gResourceTracker.ExistsTexture(id));

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

    Handle<GPUResource> ResourceManager::CreateResource(const GPUResourceDesc& desc, ResID id)
    {
       // assert(!gResourceTracker.ExistsResource(id));
        auto handle = CreateResource(desc);
        gResourceTracker.Track(id, handle);
        return handle;
    }

    Handle<MaterialInstance> ResourceManager::CreateMaterial(const PBRMaterial& matDesc, ResID id)
    {
        if (gResourceTracker.ExistsMaterial(id))
            return gResourceTracker.GetMaterialHandle(id);

        const auto getOrCreateTexture = [&](asset::AssetID asset)
        {
            if (gResourceTracker.ExistsTexture(asset.mHash))
                return gResourceTracker.GetTextureHandle(matDesc.mAlbedoTexture.mHash);

            return CreateTexture(asset);
        };

        MaterialInstance mat;
        mat.mType = MATERIAL_PBR;

        mat.mTextures[0] = getOrCreateTexture(matDesc.mAlbedoTexture);
        mat.mTextures[1] = getOrCreateTexture(matDesc.mNormalTexture);
        mat.mTextures[2] = getOrCreateTexture(matDesc.mRoughnessTexture);
        mat.mTextures[3] = getOrCreateTexture(matDesc.mMetalnessTexture);

        auto handle = mMaterialPool.Create(mat);
        gResourceTracker.Track(id, handle);

        return handle;
    }

    Handle<MaterialInstance> ResourceManager::GetMaterialHandle(ResID id)
    {
        if (gResourceTracker.ExistsMaterial(id))
        {
            return gResourceTracker.GetMaterialHandle(id);
        }

        return Null<MaterialInstance>();
    }

    Handle<Mesh> ResourceManager::GetMeshHandle(ResID id)
    {
        if (gResourceTracker.ExistsMesh(id))
        {
            return gResourceTracker.GetMeshHandle(id);
        }

        return Null<Mesh>();
    }

    Handle<Texture> ResourceManager::GetTextureHandle(ResID id)
    {
        if (gResourceTracker.ExistsTexture(id))
        {
            return gResourceTracker.GetTextureHandle(id);
        }

        return Null<Texture>();
    }

    Handle<GPUResource> ResourceManager::GetGPUResourceHandle(ResID id)
    {
        if (gResourceTracker.ExistsResource(id))
        {
            return gResourceTracker.GetGPUResourceHandle(id);
        }

        return Null<GPUResource>();
    }

    MaterialInstance* ResourceManager::GetMaterial(Handle<MaterialInstance> handle)
    {
        return mMaterialPool.Get(handle);
    }

    MaterialInstance* ResourceManager::GetMaterial(ResID id)
    {
        if (gResourceTracker.ExistsMaterial(id))
        {
            Handle<MaterialInstance> handle = gResourceTracker.GetMaterialHandle(id);
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

    void ResourceManager::ProcessAsyncLoadQueue()
    {
        using namespace asset;

        std::unique_lock<std::mutex> lock(mMutex);

        for (auto& mesh : mMeshQueue)
        {
            log::Info("[ResourceManager] Async Loading mesh: {}", StringDB::Get().GetString(mesh.mResID).c_str());
            auto asset = asset::gpAssetManager->GetAsset(asset::AssetID{ asset::ASSET_MESH, mesh.mResID });
            auto meshAsset = asset->DeserializeTo<asset::MeshAsset>();
            CreateMesh(mesh.mHandle, meshAsset.GetData());
            meshAsset.Register(mesh.mHandle);
        }

        for (auto& tex : mTextureQueue)
        {
           
        }

        mMeshQueue.clear();
        mTextureQueue.clear();
    }

    uint32_t ResourceManager::GetAsyncLoadQueueSize() const
    {
        return (uint32_t)mMeshQueue.size() + (uint32_t)mTextureQueue.size();
    }

    void ResourceManager::DestroyTexture(ResID id)
    {
        auto handle = gResourceTracker.GetTextureHandle(id);
        gResourceManager->DestroyTexture(handle);
        gResourceTracker.Remove(id, handle);
    }

    void ResourceManager::QueueDestroy(Handle<GPUResource> handle, uint32_t frameDelay)
    {
        gRenderer->QueueDestroy(handle, frameDelay);
    }

    Handle<Mesh> ResourceManager::CreateMeshAsync(ResID id)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        auto handle = EmplaceMesh();
        mMeshQueue.push_back({ handle, id });
        gResourceTracker.Track(id, handle);
        return handle;
    }

    Handle<Texture> ResourceManager::CreateTextureAsync(ResID id)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        auto handle = EmplaceTexture();
        mTextureQueue.push_back({ handle, id });
        return handle;
    }

}
