#include "pch.h"

#include <Types/Serializers.h>
#include "EntityComponent.h"
#include <Engine/Transform.h>
#include <Engine/Log.h>

#include <fstream>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>

namespace cereal
{
    using namespace nv::ecs;

    template<class Archive>
    void serialize(Archive& archive, Metadata& metadata)
    {
        using namespace cereal;
        archive(make_nvp("Components", metadata.mComponents));
    }

    template<class Archive>
    void serialize(Archive& archive, MetaField& field)
    {
        using namespace cereal;
        archive(make_nvp("Name",            field.mName),
                make_nvp("Type",            field.mType),
                make_nvp("FieldTypeEnum",   field.mFieldType));
    }
}

namespace nv::ecs
{
    using namespace cereal;

    ComponentManager gComponentManager;
    EntityManager    gEntityManager;
    std::unordered_map<std::string, StringID> gComponentNames;

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
        gComponentManager.RemoveEntityComponentMap(entity);
    }

    void EntityManager::GetEntities(nv::Vector<Handle<Entity>>& outEntities) const
    {
        mEntities.GetAllHandles(outEntities);
    }

    void EntityManager::Serialize(std::ostream& o)
    {
        {
            BinaryOutputArchive archive(o);
            archive(mRoot);
            archive(mEntityNames);
        }
        
        Serializer::Serialize(mEntities, o);
    }

    void EntityManager::Deserialize(std::istream& i)
    {
        {
            BinaryInputArchive archive(i);
            archive(mRoot);
            archive(mEntityNames);
        }

        Serializer::Deserialize(mEntities, i);
    }

    IComponent* Entity::Add(StringID compId)
    {
        if (!gComponentManager.IsPoolAvailable(compId))
        {
            log::Error("[Component] {} not found", compId);
            return nullptr;
        }

        IComponentPool* pool = gComponentManager.GetPool(compId);
        auto handle = pool->Create(mHandle);
        auto& compMap = gComponentManager.GetEntityComponentMap(mHandle);
        compMap[compId] = handle;
        return pool->GetComponent(handle);
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

    IComponent* Entity::Get(StringID compId) const
    {
        auto compMap = gComponentManager.GetEntityComponentMap(mHandle);
        auto comp = compMap.at(compId);
        IComponentPool* pool = gComponentManager.GetPool(compId);
        return pool->GetComponent(comp);
    }

    void Entity::GetComponents(std::unordered_map<StringID, IComponent*>& outComponents) const
    {
        auto& compMap = gComponentManager.GetEntityComponentMap(mHandle);
        for (auto comp : compMap)
        {
            IComponent* component = Get(comp.first);
            outComponents[comp.first] = component;
        }
    }

    ComponentManager::~ComponentManager()
    {
        for (auto& it : mComponentPools)
        {
            it.second->~IComponentPool();
        }
    }

    const std::vector<MetaField>& ComponentManager::GetMetadata(StringID compId, std::string& outName) const
    {
        const auto& key = mMetadata.mNameMap.at(compId);
        outName = key;
        const auto& fields = mMetadata.mComponents.at(outName);
        return fields;
    }

    void ComponentManager::LoadMetadata()
    {
        const char* FILENAME = "metadata.json";
        std::ifstream infile(FILENAME, std::ios::in);

        cereal::JSONInputArchive archive(infile);
        archive(mMetadata);

        for (auto& comp : mMetadata.mComponents)
        {
            auto& name = comp.first;
            auto id = ID(name);
            mMetadata.mNameMap[id] = name;
        }
    }

    void ComponentManager::Serialize(std::ostream& o)
    {
        {
            BinaryOutputArchive archive(o);
            archive(mComponentPools.size());
        }

        for (auto& pool : mComponentPools)
        {
            {
                BinaryOutputArchive archive(o);
                archive(pool.first);
            }
            pool.second->Serialize(o);
        }

        {
            BinaryOutputArchive archive(o);
            archive(mEntityComponents);
        }
    }

    void ComponentManager::Deserialize(std::istream& i)
    {
        size_t size;
        {
            BinaryInputArchive archive(i);
            archive(size);
        }

        for (size_t idx = 0; idx < size; ++idx)
        {
            StringID compId;
            {
                BinaryInputArchive archive(i);
                archive(compId);
            }
            
            mComponentPools[compId]->Deserialize(i);
        }

        {
            BinaryInputArchive archive(i);
            archive(mEntityComponents);
        }
    }

    void SerializeScene(std::ostream& o)
    {
        gEntityManager.Serialize(o);
        gComponentManager.Serialize(o);
    }

    void DeserializeScene(std::istream& i)
    {
        gEntityManager.Deserialize(i);
        gComponentManager.Deserialize(i);
    }
}
