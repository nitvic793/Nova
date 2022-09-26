#include "pch.h"
#include "System.h"
#include <Debug/Profiler.h>
#include <Lib/Format.h>

namespace nv
{
    SystemManager gSystemManager;
    SystemManager* SystemManager::gPtr = &gSystemManager;

    void SystemManager::InitSystems()
    {
        NV_EVENT("Systems/Init");
        for (auto id : mInsertOrder)
        {
            auto& system = mSystems.at(id);
            system->Init();
        }
    }

    void SystemManager::UpdateSystems(float deltaTime, float totalTime)
    {
        NV_EVENT("Systems/Update");
        for (auto id : mInsertOrder)
        {
            auto& system = mSystems.at(id);
            system->Update(deltaTime, totalTime);
        }
    }

    void SystemManager::DestroySystems()
    {
        NV_EVENT("Systems/Destroy");
        for (auto id : mInsertOrder)
        {
            auto& system = mSystems.at(id);
            system->Destroy();
        }
    }

    void SystemManager::ReloadSystems()
    {
        NV_EVENT("Systems/Reload");
        for (auto id : mInsertOrder)
        {
            auto& system = mSystems.at(id);
            system->OnReload();
        }
    }

}
