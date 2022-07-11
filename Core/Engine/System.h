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
        virtual void OnReload() {};

        virtual ~ISystem() {}
    private:
    };

    class SystemManager
    {
    public:
        using SystemPtr = ScopedPtr<ISystem, true>;

    public:
        SystemManager(IAllocator* allocator = SystemAllocator::gPtr) :
            mSystems(),
            mAllocator(allocator)
        {}

        template<typename TSystem, typename ...Args>
        ISystem* CreateSystem(Args&& ...args);

        template<typename TSystem>
        constexpr TSystem* GetSystem() const
        {
            constexpr StringID id = TypeNameID<TSystem>();
            return static_cast<TSystem*>(mSystems.at(id).Get());
        }

        ISystem* GetSystem(StringID id) const { return mSystems.at(id).Get(); }

        void RemoveSystem(StringID id)
        {
            mSystems.erase(id);
        }

        template<typename TSystem>
        constexpr void RemoveSystem()
        {
            constexpr StringID id = TypeNameID<TSystem>();
            mSystems.erase(id);
        }

        void InitSystems();
        void UpdateSystems(float deltaTime, float totalTime);
        void DestroySystems();
        void ReloadSystems();

        ~SystemManager() 
        { 
            mAllocator->Reset(); 
        };

        static SystemManager* gPtr;

    private:
        OrderedMap<StringID, ScopedPtr<ISystem, true>> mSystems;
        IAllocator* mAllocator;
    };

    extern SystemManager gSystemManager;

    template<typename TSystem, typename ...Args>
    inline ISystem* SystemManager::CreateSystem(Args&& ...args)
    {
        TSystem* buffer = (TSystem*)mAllocator->Allocate(sizeof(TSystem));
        new (buffer) TSystem(std::forward<Args>(args)...);
        constexpr StringID typeId = nv::TypeNameID<TSystem>();
        mSystems[typeId] = ScopedPtr<ISystem, true>((ISystem*)buffer);
        return (ISystem*)buffer;
    }
}

#endif // !NV_SYSTEM
