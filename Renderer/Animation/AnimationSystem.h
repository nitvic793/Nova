#pragma once

#include <Engine/System.h>
#include <Engine/EntityComponent.h>

namespace nv::graphics::animation
{
    class AnimationSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;

    private:
    };
}
