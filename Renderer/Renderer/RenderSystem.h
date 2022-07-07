#ifndef NV_RENDERER_RENDERSYSTEM
#define NV_RENDERER_RENDERSYSTEM

#pragma once

#include <Engine/System.h>

namespace nv::graphics
{
    class RenderSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;
        void OnReload() override;
    };
}

#endif // !NV_RENDERER_RENDERSYSTEM
