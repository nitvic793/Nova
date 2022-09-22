#pragma once

#include <Lib/Handle.h>
#include <Lib/Map.h>
#include <Lib/StringHash.h>

namespace nv::graphics
{
    using ResID = StringID;

    class Shader;
    class GPUResource;
    class PipelineState;
    class Texture;
    class Mesh;
    class Context;

    class ResourceTracker
    {
    public:
        void Track(ResID id, Handle<Shader>         handle);
        void Track(ResID id, Handle<GPUResource>    handle);
        void Track(ResID id, Handle<PipelineState>  handle);
        void Track(ResID id, Handle<Texture>        handle);
        void Track(ResID id, Handle<Mesh>           handle);

        void Remove(ResID id, Handle<Shader>         handle);
        void Remove(ResID id, Handle<GPUResource>    handle);
        void Remove(ResID id, Handle<PipelineState>  handle);
        void Remove(ResID id, Handle<Texture>        handle);
        void Remove(ResID id, Handle<Mesh>           handle);

        Handle<Shader>         GetShaderHandle(ResID id) const;
        Handle<GPUResource>    GetGPUResourceHandle(ResID id) const;
        Handle<PipelineState>  GetPipelineStateHandle(ResID id) const;
        Handle<Texture>        GetTextureHandle(ResID id) const;
        Handle<Mesh>           GetMeshHandle(ResID id) const;

    private:
        HashMap<ResID, Handle<Shader>       > mShaderResMap;
        HashMap<ResID, Handle<GPUResource>  > mGpuResourceResMap;
        HashMap<ResID, Handle<PipelineState>> mPsoResMap;
        HashMap<ResID, Handle<Texture>      > mTextureResMap;
        HashMap<ResID, Handle<Mesh>         > mMeshResMap;
    };
}