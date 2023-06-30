#include "pch.h"

#include <Engine/System.h>

#include "DriverSystem.h"
#include "CameraSystem.h"
#include "Player.h"

namespace nv
{
    void RegisterGameSystems()
    {
        gSystemManager.CreateSystem<CameraSystem>();
        gSystemManager.CreateSystem<DriverSystem>();
        gSystemManager.CreateSystem<PlayerController>();
    }
}