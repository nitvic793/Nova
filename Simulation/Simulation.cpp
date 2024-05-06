#include "pch.h"
#include "Simulation.h"

#include <Engine/System.h>
#include <Systems/SimDriver.h>
#include <memory>

using SysMgrUniquePtr = std::unique_ptr<nv::SystemManager>;
static SysMgrUniquePtr spSystemManager = nullptr;

void NVSimInit()
{
    spSystemManager = std::make_unique<nv::SystemManager>();
    spSystemManager->CreateSystem<nv::SimDriver>();
    spSystemManager->InitSystems();
}

void NVSimTick(float deltaTime, float totalTime)
{
    spSystemManager->UpdateSystems(deltaTime, totalTime);
}

int NVSimTickState()
{
    return 0;
}
