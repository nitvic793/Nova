#ifndef NV_SYSTEM
#define NV_SYSTEM

#pragma once

#include <Lib/Map.h>
#include <Lib/StringHash.h>
#include <Lib/Pool.h>
#include <Lib/ScopedPtr.h>

namespace nv
{
    class ISystem
    {
    public:
        virtual void Init() {}
        virtual void Update(float deltaTime, float totalTime) {}
        virtual void Destroy() {};
        virtual const char* GetName() { return ""; }

        virtual ~ISystem() {}
    private:
    };

    class SystemManager
    {
    public:
        using SystemPtr = ScopedPtr<ISystem, true>;

    public:
        SystemManager(IAllocator* allocator = SystemAllocator::gPtr)
        {}

        template<typename TSystem, typename ...Args>
        ISystem* CreateSystem(Args&& ...args);

        template<typename TSystem>
        constexpr TSystem* GetSystem() const
        {
            const StringID id = ID(TSystem);
            return static_cast<TSystem*>(mSystems.at(id).Get());
        }

        constexpr ISystem* GetSystem(StringID id) const { return mSystems.at(id).Get(); }

        ~SystemManager() { mAllocator->Reset(); };

    private:
        HashMap<StringID, ScopedPtr<ISystem>> mSystems;
        IAllocator* mAllocator;
    };

    template<typename TSystem, typename ...Args>
    inline ISystem* SystemManager::CreateSystem(Args&& ...args)
    {
        TSystem* buffer = (TSystem*)mAllocator->Allocate(sizeof(TSystem));
        new (buffer) TSystem(std::forward<Args>(args)...);
        mSystems.insert(ID(TSystem), ScopedPtr<ISystem>(buffer));
        return (ISystem*)buffer;
    }
}

#endif // !NV_SYSTEM
