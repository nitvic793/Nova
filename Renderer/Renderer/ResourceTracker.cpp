#include "pch.h"
#include "ResourceTracker.h"

namespace nv::graphics
{
    void ResourceTracker::Track(ResID id, Handle<Shader> handle)
    {
        mShaderResMap[id] = handle;
    }

    void ResourceTracker::Track(ResID id, Handle<GPUResource> handle)
    {
        mGpuResourceResMap[id] = handle;
    }

    void ResourceTracker::Track(ResID id, Handle<PipelineState> handle)
    {
        mPsoResMap[id] = handle;
    }

    void ResourceTracker::Track(ResID id, Handle<Texture> handle)
    {
        mTextureResMap[id] = handle;
    }

    void ResourceTracker::Track(ResID id, Handle<Mesh> handle)
    {
        mMeshResMap[id] = handle;
    }

    void ResourceTracker::Track(ResID id, Handle<Material> handle)
    {
        mMaterialResMap[id] = handle;
    }

    void ResourceTracker::Remove(ResID id, Handle<Shader> handle)
    {
        mShaderResMap.erase(id);
    }

    void ResourceTracker::Remove(ResID id, Handle<GPUResource> handle)
    {
        mGpuResourceResMap.erase(id);
    }

    void ResourceTracker::Remove(ResID id, Handle<PipelineState> handle)
    {
        mPsoResMap.erase(id);
    }

    void ResourceTracker::Remove(ResID id, Handle<Texture> handle)
    {
        mTextureResMap.erase(id);
    }

    void ResourceTracker::Remove(ResID id, Handle<Mesh> handle)
    {
        mMeshResMap.erase(id);
    }

    bool ResourceTracker::ExistsTexture(ResID id) const
    {
        return mTextureResMap.find(id) != mTextureResMap.end();
    }

    bool ResourceTracker::ExistsMaterial(ResID id) const
    {
        return mMaterialResMap.find(id) != mMaterialResMap.end();
    }

    Handle<Shader> ResourceTracker::GetShaderHandle(ResID id) const
    {
        return mShaderResMap.at(id);
    }

    Handle<GPUResource> ResourceTracker::GetGPUResourceHandle(ResID id) const
    {
        return mGpuResourceResMap.at(id);
    }

    Handle<PipelineState> ResourceTracker::GetPipelineStateHandle(ResID id) const
    {
        return mPsoResMap.at(id);
    }

    Handle<Texture> ResourceTracker::GetTextureHandle(ResID id) const
    {
        return mTextureResMap.at(id);
    }

    Handle<Mesh> ResourceTracker::GetMeshHandle(ResID id) const
    {
        return mMeshResMap.at(id);
    }

    Handle<Material> ResourceTracker::GetMaterialHandle(ResID id) const
    {
        return mMaterialResMap.at(id);
    }
}

