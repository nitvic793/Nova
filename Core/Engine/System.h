#ifndef NV_SYSTEM
#define NV_SYSTEM

#pragma once

#include <Lib/Map.h>
#include <Lib/StringHash.h>
#include <Lib/Pool.h>

namespace nv
{
    class ISystem
    {
    public:
        virtual void Init() {}
        virtual void Update() {}
        virtual void Destroy() {};
        virtual const char* GetName() { return ""; }

        virtual ~ISystem() {}
    private:
    };

    class SystemManager
    {
    public:
    private:
        HashMap<StringID, ISystem*> mSystems;
    };
}

#endif // !NV_SYSTEM
