#ifndef NV_RENDERER_RENDERSYSTEM
#define NV_RENDERER_RENDERSYSTEM

#pragma once

#include <Engine/System.h>

namespace nv::jobs
{
    class Job;
}

namespace nv::graphics
{
    class RenderSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;
        void OnReload() override;

        void RenderThreadJob(void* ctx);
    private:
        Handle<jobs::Job> mRenderJobHandle;
    };
}

#endif // !NV_RENDERER_RENDERSYSTEM
