#ifndef NV_RENDERER_RENDERSYSTEM
#define NV_RENDERER_RENDERSYSTEM

#pragma once

#include <Renderer/CommonDefines.h>
#include <Engine/System.h>
#include <Engine/Camera.h>
#include <Lib/Pool.h>
#include <ShaderInteropTypes.h>

namespace nv::jobs
{
    class Job;
}

namespace nv::graphics
{
    class Mesh;
    class PipelineState;

    struct DrawData
    {
        ObjectData          mData;
        ConstantBufferView  mCBView;
    };

    class RenderSystem : public ISystem
    {
    public:
        RenderSystem(uint32_t width, uint32_t height);

        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;
        void OnReload() override;

        void RenderThreadJob(void* ctx);

    private:
        void UploadDrawData();

    private:
        Pool<DrawData>      mObjectDataPool;
        Handle<jobs::Job>   mRenderJobHandle;
        
        /*TEMP*/
        Viewport            mViewport;
        Rect                mRect;
        Camera              mCamera;

        ConstantBufferView  mFrameCB;
        DrawData            mObjectDrawData;
        Handle<Mesh>        mMesh;
        Handle<PipelineState> mPso;
    };
}

#endif // !NV_RENDERER_RENDERSYSTEM
