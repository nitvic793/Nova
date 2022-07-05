
#include "Context.h"
#include "Memory/Memory.h"
#include "Memory/Allocator.h"

using namespace nv;

namespace nv
{
    Context gContext;

    Context* Context::gPtr = &gContext;

    void InitContext(Context* pContext)
    {
        InitMemoryTracker();
        pContext->mpMemTracker = GetMemoryTracker();
    }

    void DestroyContext(Context* pContext)
    {
        DestroyMemoryTracker(pContext->mpMemTracker);
    }
}
