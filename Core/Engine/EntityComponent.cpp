#include "pch.h"
#include "EntityComponent.h"

namespace nv::ecs
{
    ComponentManager gComponentManager;
    EntityManager    gEntityManager;

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
