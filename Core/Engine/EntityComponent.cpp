#include "pch.h"
#include "EntityComponent.h"

namespace nv::ecs
{
    ComponentManager gComponentManager;
    EntityManager    gEntityManager;

    void EntityManager::Init()
    {
        mEntities.Init();
    }

    void EntityManager::Destroy()
    {
        mEntities.Destroy();
    }

    Handle<Entity> EntityManager::Create()
    {
        auto handle = mEntities.Create();

        return handle;
    }

    void EntityManager::Remove(Handle<Entity> entity)
    {
        mEntities.Remove(entity);
    }
}
