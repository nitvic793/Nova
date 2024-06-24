#ifndef NV_SYSTEM
#define NV_SYSTEM

#pragma once

#include <Lib/Handle.h>
#include <Lib/Map.h>
#include <Lib/StringHash.h>
#include <Lib/Pool.h>
#include <Lib/ScopedPtr.h>
#include <mutex>

namespace nv
{
#if _DEBUG || NV_ENABLE_PROFILING
#define NV_ENABLE_SYSTEM_NAMES 1
    constexpr bool ENABLE_SYSTEM_NAMES = true;
#else
    constexpr bool ENABLE_SYSTEM_NAMES = false;
#endif

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
            std::unique_lock<std::mutex> lock(mSysMutex);
            constexpr StringID id = TypeNameID<TSystem>();
            return static_cast<TSystem*>(mSystems.at(id).Get());
        }

        ISystem* GetSystem(StringID id) const { return mSystems.at(id).Get(); }

        void RemoveSystem(StringID id)
        {
            std::unique_lock<std::mutex> lock(mSysMutex);
            mSystems.erase(id);
        }

        template<typename TSystem>
        constexpr void RemoveSystem()
        {
            std::unique_lock<std::mutex> lock(mSysMutex);
            constexpr StringID id = TypeNameID<TSystem>();
            mSystems.erase(id);
        }

        void InitSystems();
        void UpdateSystems(float deltaTime, float totalTime);
        void DestroySystems();
        void ReloadSystems();

        void SetSystemName(StringID id, const std::string_view& name)
        {
            std::unique_lock<std::mutex> lock(mSysMutex);
            if constexpr (ENABLE_SYSTEM_NAMES)
                mSystemNames[id] = name;
        }

        const char* GetSystemName(StringID id)
        {
            std::unique_lock<std::mutex> lock(mSysMutex);
            if constexpr (ENABLE_SYSTEM_NAMES)
                return mSystemNames[id].c_str();
        }

        ~SystemManager() 
        { 
            mAllocator->Reset(); 
        };

        static SystemManager* gPtr;

    private:
        nv::Vector<StringID>                        mInsertOrder;
        HashMap<StringID, ScopedPtr<ISystem, true>> mSystems;
        mutable std::mutex                          mSysMutex; // Committing a grave sin, but const functions need mutex lock for safety
#if NV_ENABLE_SYSTEM_NAMES
        HashMap<StringID, std::string> mSystemNames;
#endif

        IAllocator* mAllocator;
    };

    extern SystemManager gSystemManager;

    template<typename TSystem, typename ...Args>
    inline ISystem* SystemManager::CreateSystem(Args&& ...args)
    {
        TSystem* buffer = (TSystem*)mAllocator->Allocate(sizeof(TSystem));
        new (buffer) TSystem(std::forward<Args>(args)...);
        constexpr auto typeName = nv::TypeName<TSystem>();
        constexpr StringID typeId = nv::TypeNameID<TSystem>();
        mSystems[typeId] = ScopedPtr<ISystem, true>((ISystem*)buffer);
        SetSystemName(typeId, typeName);
        mInsertOrder.Push(typeId);
        return (ISystem*)buffer;
    }
}

#endif // !NV_SYSTEM
