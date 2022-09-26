#include "pch.h"
#include "EntityComponent.h"
#include <Engine/Transform.h>

namespace nv::ecs
{
    ComponentManager gComponentManager;
    EntityManager    gEntityManager;

    void EntityManager::Init()
    {
        mEntities.Init();
        mRoot = mEntities.Create();
    }

    void EntityManager::Destroy()
    {
        mEntities.Destroy();
    }

    Handle<Entity> EntityManager::Create(Handle<Entity> parent)
    {
        if (parent == Null<Entity>())
        {
            parent = mRoot;
        }

        auto handle = mEntities.Create();
        Entity* entity = mEntities.Get(handle);
        entity->mHandle = handle;
        entity->mParent = parent;
        return handle;
    }

    void EntityManager::Remove(Handle<Entity> entity)
    {
        mEntities.Remove(entity);
    }

    void Entity::AttachTransform(const Transform& transform)
    {
        auto pos = Add<Position>();
        auto rot = Add<Rotation>();
        auto scale = Add<Scale>();

        pos->mPosition = transform.mPosition;
        rot->mRotation = transform.mRotation;
        scale->mScale = transform.mScale;
    }

    ComponentManager::~ComponentManager()
    {
        for (auto& it : mComponentPools)
        {
            it.second->~IComponentPool();
        }
    }
}
