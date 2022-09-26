#include "pch.h"

#include <Engine/System.h>

#include "DriverSystem.h"

namespace nv
{
    void RegisterGameSystems()
    {
        gSystemManager.CreateSystem<DriverSystem>();
    }
}