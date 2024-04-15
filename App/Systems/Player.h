#pragma once

#include <Engine/System.h>
#include <Engine/EntityComponent.h>

#include "EntityCommon.h"

namespace nv
{
    class PlayerController : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;

        void OnFrameRecordStateChange(FrameRecordEvent* pEvent);

    private:
        Handle<ecs::Entity> mPlayerEntity;
    };

    ecs::Entity* GetPlayerEntity();
}
