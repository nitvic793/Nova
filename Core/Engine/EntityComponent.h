#ifndef NV_ENGINE_ENTITYCOMPONENT
#define NV_ENGINE_ENTITYCOMPONENT

#pragma once

#include <Lib/Handle.h>
#include <Lib/Vector.h>
#include <Lib/Pool.h>
#include <Lib/Map.h>
#include <Lib/ScopedPtr.h>

namespace nv
{
    struct Entity;
    struct IComponent {};

    template<typename T>
    struct CompHandle : public Handle<IComponent>{};

    class IComponentPool 
    {
    public:
        
    };

    template<typename TComp>
    class ComponentPool : public IComponentPool
    {
    public:


    private:
        Pool<TComp> mComponents;
        HashMap<Handle<Entity>, Handle<TComp>> mEntityCompMap;
    };

    class ComponentManager
    {
    public:

    private:
        HashMap<StringID, ScopedPtr<IComponentPool>> mComponentPools;
    };

    struct Entity
    {
    public:


    public:
        Handle<Entity>      mHandle;
        Vector<StringID>    mComponents;
    };

    class EntityManager
    {
    public:
        constexpr Entity* GetEntity(Handle<Entity> handle) const { return mEntities.Get(handle); }
    private:
        Pool<Entity> mEntities;
        ComponentManager mComponentMgr;
    };
}

#endif // !NV_ENGINE_ENTITYCOMPONENT
