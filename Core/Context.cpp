
#include "Context.h"
#include "Memory/Memory.h"
#include "Memory/Allocator.h"
#include <Engine/System.h>
#include <Engine/JobSystem.h>
#include <Asset.h>
#include <AssetManager.h>

using namespace nv;

namespace nv
{
    constexpr uint32_t  NV_JOB_WORKER_THREAD_COUNT = 4;
    constexpr char      NV_DATA_PATH[] = "\\Data";

    Context gContext;
    Context* Context::gPtr = &gContext;

    void InitContext(Instance* pInstance, Context* pContext)
    {
        InitMemoryTracker();
        pContext->mpMemTracker = GetMemoryTracker();
        pContext->mpInstance = pInstance;
        pContext->mpSystemManager = &gSystemManager;

        jobs::InitJobSystem(NV_JOB_WORKER_THREAD_COUNT);
        asset::InitAssetManager(NV_DATA_PATH);

        pContext->mpAssetManager = asset::GetAssetManager();
        pContext->mpAssetManager->LoadAsset({ asset::ASSET_MESH, ID("Mesh/cube.obj") });
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
