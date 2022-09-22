#ifndef NV_RENDERER_RENDERSYSTEM
#define NV_RENDERER_RENDERSYSTEM

#pragma once

#include <Renderer/CommonDefines.h>
#include <Engine/System.h>
#include <Engine/Camera.h>
#include <Lib/ConcurrentQueue.h>
#include <Lib/Map.h>
#include <Interop/ShaderInteropTypes.h>

namespace nv::jobs
{
    class Job;
}

namespace nv::graphics
{
    class Mesh;
    class PipelineState;
    class RenderReloadManager;
    class Texture;
    class ConstantBufferPool;

    struct DrawData
    {
        ObjectData          mData;
        MaterialData        mMaterial;
        ConstantBufferView  mObjectCBView;
        ConstantBufferView  mMaterialCBView;
    };

    class RenderSystem : public ISystem
    {
    public:
        RenderSystem(uint32_t width, uint32_t height);

        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;
        void OnReload() override;

        void QueueReload(Handle<PipelineState>* pso);
        void Reload(Handle<PipelineState>* pso);

        void RenderThreadJob(void* ctx);

    private:
        void UploadDrawData();

    private:
        Handle<jobs::Job>   mRenderJobHandle;
        ConstantBufferPool* mpConstantBufferPool;
        
        /*TEMP*/
        Viewport                mViewport;
        Rect                    mRect;
        Camera                  mCamera;

        ConstantBufferView      mFrameCB;
        DrawData                mObjectDrawData;
        Handle<Mesh>            mMesh;
        Handle<PipelineState>   mPso;
        Handle<Texture>         mTexture;

        RenderReloadManager* mReloadManager;
        ConcurrentQueue<Handle<PipelineState>*> mPsoReloadQueue;
    };
}

#endif // !NV_RENDERER_RENDERSYSTEM
