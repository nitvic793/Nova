
#include "Context.h"
#include "Memory/Memory.h"
#include "Memory/Allocator.h"
#include <Engine/System.h>
#include <Engine/JobSystem.h>
#include <Asset.h>
#include <AssetManager.h>
#include <AssetSystem.h>

using namespace nv;

namespace nv
{
    Context gContext;
    Context* Context::gPtr = &gContext;

    void InitContext(Instance* pInstance, const char* pDataPath, Context* pContext)
    {
        InitMemoryTracker();
        pContext->mpMemTracker = GetMemoryTracker();
        pContext->mpInstance = pInstance;
        pContext->mpSystemManager = &gSystemManager;

        jobs::InitJobSystem(NV_JOB_WORKER_THREAD_COUNT);
        asset::InitAssetManager(pDataPath);

        pContext->mpAssetManager = asset::GetAssetManager();

        gSystemManager.CreateSystem<asset::AssetSystem>();
        //auto handle = pContext->mpAssetManager->ExportAssets(".\\Build\\Assets.novapkg");
        //jobs::Wait(handle);
    }

    void DestroyContext(Context* pContext)
    {
        DestroyMemoryTracker(pContext->mpMemTracker);
        MemTracker::gPtr = nullptr;
        pContext->mpInstance = nullptr;
        jobs::DestroyJobSystem();
        asset::DestroyAssetManager();
    }

    void ApplyContext(Context* pContext)
    {
        Context::gPtr = pContext;
        gContext = *pContext;
    }
}
