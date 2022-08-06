
#include "Context.h"
#include "Memory/Memory.h"
#include "Memory/Allocator.h"
#include <Engine/System.h>
#include <Engine/JobSystem.h>

using namespace nv;

namespace nv
{
    constexpr uint32_t NV_JOB_WORKER_THREAD_COUNT = 4;

    Context gContext;
    Context* Context::gPtr = &gContext;

    void InitContext(Instance* pInstance, Context* pContext)
    {
        InitMemoryTracker();
        pContext->mpMemTracker = GetMemoryTracker();
        pContext->mpInstance = pInstance;
        pContext->mpSystemManager = &gSystemManager;
        jobs::InitJobSystem(NV_JOB_WORKER_THREAD_COUNT);
    }

    void DestroyContext(Context* pContext)
    {
        DestroyMemoryTracker(pContext->mpMemTracker);
        MemTracker::gPtr = nullptr;
        pContext->mpInstance = nullptr;
        jobs::DestroyJobSystem();
    }

    void ApplyContext(Context* pContext)
    {
        Context::gPtr = pContext;
        gContext = *pContext;
    }
}
