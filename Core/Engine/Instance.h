#ifndef NV_ENGINE_INSTANCE
#define NV_ENGINE_INSTANCE

#pragma once

namespace nv
{
    class Instance
    {
    public:
        Instance(const char* appName) :
            mAppName(appName) {}
        bool Init();
        bool Run();
        bool Destroy();
        void Reload();

    protected:
        const char* mAppName;
    };
}

#endif // !NV_ENGINE_INSTANCE