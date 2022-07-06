#include "pch.h"
#include "System.h"

namespace nv
{
    SystemManager gSystemManager;
    SystemManager* SystemManager::gPtr = &gSystemManager;

    void SystemManager::InitSystems()
    {
        for (auto& system : mSystems)
        {
            system.second->Init();
        }
    }

    void SystemManager::UpdateSystems(float deltaTime, float totalTime)
    {
        for (auto& system : mSystems)
        {
            system.second->Update(deltaTime, totalTime);
        }
    }

    void SystemManager::DestroySystems()
    {
        for (auto& system : mSystems)
        {
            system.second->Destroy();
        }
    }

}
