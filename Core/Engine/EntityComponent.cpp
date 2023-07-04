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

    Handle<Entity> EntityManager::Create(Handle<Entity> parent, const char* pDebugName)
    {
        if (parent.IsNull())
        {
            parent = mRoot;
        }

        auto handle = mEntities.Create();
        Entity* entity = mEntities.Get(handle);
        entity->mHandle = handle;
        entity->mParent = parent;

        std::string entityName;
        if (!pDebugName)
            entityName = "Entity " + std::to_string(handle.mIndex);
        else
            entityName = pDebugName;

        mEntityNames[entityName] = handle;

        return handle;
    }

    void EntityManager::Remove(Handle<Entity> entity)
    {
        mEntities.Remove(entity);
    }

    void EntityManager::GetEntities(Vector<Handle<Entity>>& outEntities) const
    {
        mEntities.GetAllHandles(outEntities);
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

    TransformRef Entity::GetTransform() const
    {
        auto pos = Get<Position>();
        auto rot = Get<Rotation>();
        auto scale = Get<Scale>();
        return TransformRef
        { 
            .mPosition = pos->mPosition, 
            .mRotation = rot->mRotation, 
            .mScale = scale->mScale 
        };
    }

    ComponentManager::~ComponentManager()
    {
        for (auto& it : mComponentPools)
        {
            it.second->~IComponentPool();
        }
    }
}
