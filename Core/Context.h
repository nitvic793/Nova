#pragma once

namespace nv
{
    class MemTracker;
    class SystemAllocator;
    class SystemManager;
    class Instance;
    namespace jobs
    {
        class IJobSystem;
    }

    struct Context
    {
        MemTracker*         mpMemTracker;
        SystemManager*      mpSystemManager;
        Instance*           mpInstance;
        jobs::IJobSystem*   mpJobSystem;
        static Context*     gPtr;
    };

    extern Context gContext;

    void InitContext(Instance* pInstance, Context* pContext = Context::gPtr);
    void DestroyContext(Context* pContext = Context::gPtr);

    void ApplyContext(Context* pContext);
}