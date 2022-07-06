#pragma once

namespace nv
{
    class MemTracker;
    class SystemAllocator;
    class SystemManager;

    struct Context
    {
        MemTracker*         mpMemTracker;
        SystemManager*      mpSystemManager;
        static Context*     gPtr;
    };

    extern Context gContext;

    void InitContext(Context* pContext = Context::gPtr);
    void DestroyContext(Context* pContext = Context::gPtr);

    void ApplyContext(Context* pContext);
}