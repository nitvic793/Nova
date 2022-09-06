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
        for (auto& system : mSystems)
        {
            const auto event = nv::Format("System/Init/{}", GetSystemName(system.first));
            NV_EVENT(event.c_str());
            system.second->Init();
        }
    }

    void SystemManager::UpdateSystems(float deltaTime, float totalTime)
    {
        for (auto& system : mSystems)
        {
            const auto event = nv::Format("System/Update/{}", GetSystemName(system.first));
            NV_EVENT(event.c_str());
            system.second->Update(deltaTime, totalTime);
        }
    }

    void SystemManager::DestroySystems()
    {
        for (auto& system : mSystems)
        {
            const auto event = nv::Format("System/Destroy/{}", GetSystemName(system.first));
            NV_EVENT(event.c_str());
            system.second->Destroy();
        }
    }

    void SystemManager::ReloadSystems()
    {
        for (auto& system : mSystems)
        {
            const auto event = nv::Format("System/Reload/{}", GetSystemName(system.first));
            NV_EVENT(event.c_str());
            system.second->OnReload();
        }
    }

}
