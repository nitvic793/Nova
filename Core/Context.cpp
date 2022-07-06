
#include "Context.h"
#include "Memory/Memory.h"
#include "Memory/Allocator.h"
#include <Engine/System.h>

using namespace nv;

namespace nv
{
    Context gContext;
    Context* Context::gPtr = &gContext;

    void InitContext(Instance* pInstance, Context* pContext)
    {
        InitMemoryTracker();
        pContext->mpMemTracker = GetMemoryTracker();
        pContext->mpInstance = pInstance;
        pContext->mpSystemManager = &gSystemManager;
    }

    void DestroyContext(Context* pContext)
    {
        DestroyMemoryTracker(pContext->mpMemTracker);
        pContext->mpInstance = nullptr;
    }

    void ApplyContext(Context* pContext)
    {
        Context::gPtr = pContext;
        gContext = *pContext;
    }
}
