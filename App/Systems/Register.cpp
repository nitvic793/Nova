#include "pch.h"

#include <Engine/System.h>

#include "DriverSystem.h"
#include "CameraSystem.h"
#include "Player.h"
#include "EntityCommon.h"

namespace nv
{
    void RegisterGameSystems()
    {
        gSystemManager.CreateSystem<FramePreSystem>();
        gSystemManager.CreateSystem<CameraSystem>();
        gSystemManager.CreateSystem<DriverSystem>();
        gSystemManager.CreateSystem<PlayerController>();
    }
}