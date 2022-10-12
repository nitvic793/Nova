#pragma once

#include <Engine/System.h>
#include <Input/InputState.h>
#include <Math/Math.h>

namespace nv
{
    namespace ecs
    {
        struct Entity;
    }

    class CameraSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;

    private:
        Handle<ecs::Entity> mEditorCamera;
        math::float2 mPrevPos;
    };
}
