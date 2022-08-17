#pragma once

namespace nv
{
    constexpr uint32_t  NV_JOB_WORKER_THREAD_COUNT = 4;
    constexpr char      NV_DATA_PATH[] = "\\Build";

    class MemTracker;
    class SystemAllocator;
    class SystemManager;
    class Instance;
    namespace jobs
    {
        class IJobSystem;
    }

    namespace asset
    {
        class IAssetManager;
    }

    struct Context
    {
        MemTracker*             mpMemTracker;
        SystemManager*          mpSystemManager;
        Instance*               mpInstance;
        jobs::IJobSystem*       mpJobSystem;
        asset::IAssetManager*   mpAssetManager;
        static Context*         gPtr;
    };

    extern Context gContext;

    void InitContext(Instance* pInstance, const char* pDataPath = NV_DATA_PATH, Context* pContext = Context::gPtr);
    void DestroyContext(Context* pContext = Context::gPtr);

    void ApplyContext(Context* pContext);
}