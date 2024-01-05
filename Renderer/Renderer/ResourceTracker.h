#pragma once

#include <Lib/Handle.h>
#include <Lib/Map.h>
#include <Lib/StringHash.h>

#include <Renderer/CommonDefines.h>

namespace nv::graphics
{
    class Shader;
    class GPUResource;
    class PipelineState;
    class Texture;
    class Mesh;
    class Context;
    struct Material;

    class ResourceTracker
    {
    public:
        void Track(ResID id, Handle<Shader>         handle);
        void Track(ResID id, Handle<GPUResource>    handle);
        void Track(ResID id, Handle<PipelineState>  handle);
        void Track(ResID id, Handle<Texture>        handle);
        void Track(ResID id, Handle<Mesh>           handle);
        void Track(ResID id, Handle<Material>       handle);

        void Remove(ResID id, Handle<Shader>         handle);
        void Remove(ResID id, Handle<GPUResource>    handle);
        void Remove(ResID id, Handle<PipelineState>  handle);
        void Remove(ResID id, Handle<Texture>        handle);
        void Remove(ResID id, Handle<Mesh>           handle);

        void Remove(Handle<GPUResource>    handle);

        bool ExistsTexture(ResID id) const;
        bool ExistsMaterial(ResID id) const;
        bool ExistsMesh(ResID id) const;
        bool ExistsShader(ResID id) const;
        bool ExistsResource(ResID id) const;

        Handle<Shader>         GetShaderHandle(ResID id) const;
        Handle<GPUResource>    GetGPUResourceHandle(ResID id) const;
        Handle<PipelineState>  GetPipelineStateHandle(ResID id) const;
        Handle<Texture>        GetTextureHandle(ResID id) const;
        Handle<Mesh>           GetMeshHandle(ResID id) const;
        Handle<Material>       GetMaterialHandle(ResID id) const;

    private:
        HashMap<ResID, Handle<Shader>       > mShaderResMap;
        HashMap<ResID, Handle<GPUResource>  > mGpuResourceResMap;
        HashMap<ResID, Handle<PipelineState>> mPsoResMap;
        HashMap<ResID, Handle<Texture>      > mTextureResMap;
        HashMap<ResID, Handle<Mesh>         > mMeshResMap;
        HashMap<ResID, Handle<Material>     > mMaterialResMap;
    };
}