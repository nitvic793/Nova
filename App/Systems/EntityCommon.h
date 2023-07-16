#pragma once

#include <Renderer/CommonDefines.h>
#include <Engine/Transform.h>
#include <Lib/Handle.h>
#include <Engine/EntityComponent.h>

#include <Engine/EventSystem.h>

namespace nv
{
    enum FrameRecordState 
    {
        FRAME_RECORD_STOPPED        = 0,
        FRAME_RECORD_IN_PROGRESS    = 1,
        FRAME_RECORD_REWINDING      = 2
    };

    struct FrameRecordEvent : public IEvent
    {
        FrameRecordState mState;
    };

    Handle<ecs::Entity> CreateEntity(graphics::ResID mesh, graphics::ResID mat, const char* pDebugName = nullptr, const Transform& transform = Transform());

    bool IsDebugUIEnabled();
}