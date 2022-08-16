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

    public:
        static void SetError(bool isError, const char* pReason);
        static bool HasError() { return sError; }
        static const char* GetErrorReason() { return spErrorReason; }

    protected:
        bool UpdateSystemState() const;

    protected:
        const char* mAppName;

    protected:
        static bool sError;
        static const char* spErrorReason;
    };
}

#endif // !NV_ENGINE_INSTANCE