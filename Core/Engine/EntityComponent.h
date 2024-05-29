#ifndef NV_ENGINE_ENTITYCOMPONENT
#define NV_ENGINE_ENTITYCOMPONENT

#pragma once

#include <Lib/Handle.h>
#include <Lib/Vector.h>
#include <Lib/Pool.h>
#include <Lib/Map.h>
#include <Lib/ScopedPtr.h>
#include <Lib/Serializer.h>

#include <Engine/Component.h>
#include <Engine/Transform.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/cereal.hpp>

namespace nv::ecs
{
    struct Entity;

    // Need to keep in sync with FieldType enum in Scripts/components_common.py
    enum FieldType : uint8_t
    {
        FIELD_UNDEFINED = 0,
        FIELD_FLOAT,
        FIELD_INT,
        FIELD_FLOAT2,
        FIELD_FLOAT3,
        FIELD_FLOAT4,
        FIELD_STRING,
        FIELD_HANDLE_TEX = 7,
        FIELD_HANDLE_MESH = 8,
        FIELD_HANDLE_MAT = 9,
        FIELD_UINT = 10,
        FIELD_BOOL = 11,
        FIELD_INT64 = 12,
        FIELD_UINT64 = 13,
        FIELD_COUNT
    };

    enum FieldTypeSize : size_t
    {
        FIELD_UNDEFINED_SIZE    = 0,
        FIELD_FLOAT_SIZE        = sizeof(float),
        FIELD_INT_SIZE          = sizeof(int32_t),
        FIELD_FLOAT2_SIZE       = sizeof(math::float2),
        FIELD_FLOAT3_SIZE       = sizeof(math::float3),
        FIELD_FLOAT4_SIZE       = sizeof(math::float4),
        FIELD_STRING_SIZE       = sizeof(std::string),
        FIELD_HANDLE_TEX_SIZE   = sizeof(Handle<int32_t>),
        FIELD_HANDLE_MESH_SIZE  = sizeof(Handle<int32_t>),
        FIELD_HANDLE_MAT_SIZE   = sizeof(Handle<int32_t>),
        FIELD_UINT_SIZE         = sizeof(uint32_t),
        FIELD_BOOL_SIZE         = sizeof(bool),
        FIELD_INT64_SIZE        = sizeof(int64_t),
        FIELD_UINT64_SIZE       = sizeof(uint64_t)
    };

    static constexpr size_t gFieldSizeMap[FIELD_COUNT] =
    {
        FIELD_UNDEFINED_SIZE,
        FIELD_FLOAT_SIZE ,
        FIELD_INT_SIZE   ,
        FIELD_FLOAT2_SIZE,
        FIELD_FLOAT3_SIZE,
        FIELD_FLOAT4_SIZE,
        FIELD_STRING_SIZE,
        FIELD_HANDLE_TEX_SIZE,
        FIELD_HANDLE_MESH_SIZE,
        FIELD_HANDLE_MAT_SIZE,
        FIELD_UINT_SIZE,
        FIELD_BOOL_SIZE,
        FIELD_INT64_SIZE,
        FIELD_UINT64_SIZE
    };

    struct Field
    {
        std::string mFieldName;
        FieldType   mType       = FIELD_UNDEFINED;
        uint32_t    mOffset     = 0;
    };

    struct MetaField
    {
        std::string mName;
        std::string mType;
        nv::ecs::FieldType mFieldType;
    };

    struct Metadata
    {
        std::unordered_map<std::string, std::vector<MetaField>> mComponents;
        std::unordered_map<StringID, std::string_view> mNameMap;
    };

    template<typename TComp>
    struct EntityComponents
    {
        struct Instance
        {
            Entity* mpEntity;
            TComp* mpComponent;
        };

        nv::Vector<Handle<Entity>> mEntities;
        nv::Vector<TComp*> mComponents;

        constexpr size_t Size() const { return mEntities.size(); }

        Instance operator[](size_t idx) const;
    };

    class IComponentPool 
    {
    public:
        IComponentPool(Span<Field> fields) 
        {
            mFields.Reserve((uint32_t)fields.Size());
            for (const Field& field : fields)
                mFields.Push(field);
        }

        virtual bool                IsValid(uint64_t handle) const = 0;
        virtual Span<IComponent>    GetComponents() const = 0;
        virtual IComponent*         GetComponent(uint32_t index) const = 0;
        virtual IComponent*         GetComponent(uint64_t handle) const = 0;
        virtual size_t              GetStrideSize() const = 0;
        virtual size_t              Size() const = 0;
        virtual uint64_t            Create(Handle<Entity> entity) = 0;
        virtual void                Remove(uint64_t handle) = 0;
        virtual void                RemoveEntity(Handle<Entity> entity) = 0;
        virtual void                SerializeForFrame(std::ostream& ostream) = 0;
        virtual void                DeserializeForFrame(std::istream& istream) = 0;
        virtual void                Serialize(std::ostream& ostream) = 0;
        virtual void                Deserialize(std::istream& istream) = 0;

        template<typename TComp>
        IComponent* GetComponent(Handle<TComp> handle)
        {
            if (!IsValid(handle.mHandle))
                return nullptr;

            return static_cast<TComp*>(GetComponent(handle.mHandle));
        }

        virtual ~IComponentPool() {}

    protected:
        nv::Vector<Field>   mFields;
    };

    template<typename TComp>
    class ComponentPool : public IComponentPool
    {
        using EntityComponentMap = UnorderedMap<uint64_t, Handle<TComp>>;

    public:
        ComponentPool(Span<Field> fields):
            IComponentPool(fields)
        {}

        virtual bool IsValid(uint64_t handle) const override
        {
            Handle<TComp> h; 
            h.mHandle = handle;
            return mComponents.IsValid(h);
        }

        virtual Span<IComponent> GetComponents() const override
        {
            nv::Span<TComp> span = mComponents.Span();
            return { (IComponent*)span.mData, span.mSize };
        }

        virtual IComponent* GetComponent(uint32_t index) const override
        {
            return (IComponent*)(&mComponents[index]);
        }

        virtual IComponent* GetComponent(uint64_t handle) const override
        {
            Handle<TComp> h;
            h.mHandle = handle;
            return static_cast<IComponent*>(mComponents.Get(h));
        }

        virtual size_t GetStrideSize() const override
        {
            return sizeof(TComp);
        }

        virtual size_t Size() const override
        {
            return mComponents.Size();
        }

        virtual uint64_t Create(Handle<Entity> entity) override
        {
            Handle<TComp> handle = mComponents.Create();
            mEntityMap[entity.mHandle] = handle;
            return handle.mHandle;
        }

        virtual void Remove(uint64_t handle) override
        {
            Handle<TComp> h;
            h.mHandle = handle;
            mComponents.Remove(h);
        }

        constexpr Span<TComp> Span() const
        {
            return mComponents.Span();
        }

        virtual void RemoveEntity(Handle<Entity> entity) override
        {
            auto handle = mEntityMap[entity.mHandle];
            mEntityMap.erase(entity.mHandle);
            Remove(handle.mHandle);
        }

        virtual void SerializeForFrame(std::ostream& ostream) override
        {
            using namespace cereal;

            BinaryOutputArchive archive(ostream);
            nv::Span<TComp> span = mComponents.Span();

            archive(GetComponentID<TComp>());
            archive(mComponents.Size());
            archive(binary_data(span.begin(), static_cast<std::size_t>(span.Size()) * sizeof(TComp)));
        }

        virtual void DeserializeForFrame(std::istream& istream) override
        {
            using namespace cereal;
            cereal::BinaryInputArchive archive(istream);

            StringID compId;
            size_t size;

            archive(compId);
            archive(size);
            assert(compId == GetComponentID<TComp>());

            void* buffer = Alloc(size * sizeof(TComp));
            archive(binary_data(buffer, static_cast<std::size_t>(size) * sizeof(TComp)));
            mComponents.CopyToPool((TComp*)buffer, size);
            Free(buffer);
        }

        virtual void Serialize(std::ostream& ostream) override
        {
            using namespace cereal;

            Serializer::Serialize(mComponents, ostream);
            BinaryOutputArchive archive(ostream);
            archive(mEntityMap);
        }

        virtual void Deserialize(std::istream& istream) override
        {
            using namespace cereal;
            Serializer::Deserialize(mComponents, istream);
            cereal::BinaryInputArchive archive(istream);
            archive(mEntityMap);
        }

        void GetEntityComponents(EntityComponents<TComp>& outEntityComponents) const
        {
            for (auto e : mEntityMap)
            {
                auto handle = Handle<Entity>{};
                TComp* const pComp = (TComp * const)mComponents.GetAsDerived(e.second);
                handle.mHandle = e.first;
                outEntityComponents.mEntities.Push(handle);
                outEntityComponents.mComponents.Push(pComp);
            }
        }

    private:
        ContiguousPool<TComp>   mComponents;
        EntityComponentMap      mEntityMap;

        friend class ComponentManager;
    };

    class ComponentManager
    {
        using ComponentPoolMap = UnorderedMap<StringID, ScopedPtr<IComponentPool, true>>;
        using EntityComponentMap = UnorderedMap<StringID, uint64_t>;

    public:
        template<typename TComp>
        ComponentPool<TComp>* GetPool() const
        {
            constexpr StringID compId = GetComponentID<TComp>();
            if (mComponentPools.find(compId) == mComponentPools.end())
                return nullptr;
            auto& pool = mComponentPools.at(compId);
            return pool.As<ComponentPool<TComp>>();
        }

        IComponentPool* GetPool(StringID compId) const
        {
            auto it = mComponentPools.find(compId);
            if (it == mComponentPools.end())
                return nullptr;

            return it->second.Get();
        }

        template<typename TComp>
        Span<TComp> GetComponents()
        {
            constexpr StringID compId = GetComponentID<TComp>();
            auto it = mComponentPools.find(compId);
            if (it == mComponentPools.end())
                return Span<TComp>::Empty();

            auto& pool = it->second;
            ComponentPool<TComp>* cPool = pool.As<ComponentPool<TComp>>();
            return cPool->mComponents.Span();
        }

        template<typename TComp>
        void CreatePool(Span<Field> fields)
        {
            constexpr auto compName = GetComponentName<TComp>();
            constexpr StringID compId = GetComponentID<TComp>();
            void* buffer = Alloc<ComponentPool<TComp>>(SystemAllocator::gPtr, fields);
            mComponentPools[compId] = ScopedPtr<IComponentPool, true>((IComponentPool*)buffer);
        }

        bool IsPoolAvailable(StringID id) const
        {
            return mComponentPools.find(id) != mComponentPools.end();
        }

        const ComponentPoolMap& GetAllPools() const
        {
            return mComponentPools;
        }

        const std::vector<MetaField>& GetMetadata(StringID compId, std::string& outName) const;

        void LoadMetadata();

        void Serialize(std::ostream& o);
        void Deserialize(std::istream& i);

        EntityComponentMap& GetEntityComponentMap(Handle<Entity> entity)
        {
            auto it = mEntityComponents.find(entity.mHandle);
            if (it == mEntityComponents.end())
                mEntityComponents[entity.mHandle] = EntityComponentMap();

            return mEntityComponents[entity.mHandle];
        }

        void RemoveEntityComponentMap(Handle<Entity> entity)
        {
            auto it = mEntityComponents.find(entity.mHandle);
            if (it != mEntityComponents.end())
            {
                mEntityComponents[entity.mHandle].clear();
                mEntityComponents.erase(it);
            }
        }

        ~ComponentManager();

    private:
        ComponentPoolMap mComponentPools;
        UnorderedMap<uint64_t, EntityComponentMap> mEntityComponents;

        Metadata mMetadata;
    };

    extern ComponentManager gComponentManager;
    extern std::unordered_map<std::string, StringID> gComponentNames;

    struct Entity
    {
    public:
        template<typename TComp, typename ...Args>
        TComp* Add(Args&&... args)
        {
            auto compName = GetComponentName<TComp>();
            constexpr StringID compId = GetComponentID<TComp>();
            gComponentNames[std::string(compName)] = compId;
            if (!gComponentManager.IsPoolAvailable(compId))
                gComponentManager.CreatePool<TComp>({ nullptr, 0 });
            
            auto& compMap = gComponentManager.GetEntityComponentMap(mHandle);

            ComponentPool<TComp>* pool = gComponentManager.GetPool<TComp>();
            auto handle = pool->Create(mHandle);
            compMap[compId] = handle;
            return pool->GetComponent(handle)->As<TComp>();
        }

        template<typename TComp>
        void Remove()
        {
            auto compName = GetComponentName<TComp>();
            constexpr StringID compId = GetComponentID<TComp>();
            gComponentNames[std::string(compName)] = compId;
            if (!gComponentManager.IsPoolAvailable(compId))
                return;

            ComponentPool<TComp>* pool = gComponentManager.GetPool<TComp>();
            pool->RemoveEntity(mHandle);
        }

        IComponent* Add(StringID compId);

        void AttachTransform(const Transform& transform = Transform());
        TransformRef GetTransform() const;

        template<typename TComp>
        TComp* Get() const
        {
            constexpr StringID compId = GetComponentID<TComp>();
            auto& compMap = gComponentManager.GetEntityComponentMap(mHandle);
            auto comp = compMap.at(compId);
            ComponentPool<TComp>* pool = gComponentManager.GetPool<TComp>();
            return pool->GetComponent(comp)->As<TComp>();
        }

        IComponent* Get(StringID compId) const;

        template<typename TComp>
        bool Has() const
        {
            constexpr StringID compId = GetComponentID<TComp>();
            auto& compMap = gComponentManager.GetEntityComponentMap(mHandle);
            auto comp = compMap.find(compId);
            return comp != compMap.end();
        }

        constexpr void SetHandle(Handle<Entity> handle) { mHandle = handle; }

        void GetComponents(std::unordered_map<StringID, IComponent*>& outComponents) const;

        template<class Archive>
        void serialize(Archive& archive) { archive(mParent, mHandle); }

    public:
        Handle<Entity> mParent;
        Handle<Entity> mHandle;
    };

    class EntityManager
    {
    public:
        void Init();
        void Destroy();

        // TODO:
        // Test Transform Creation
        // Pass Transform data to renderer 
        // Attach "Renderable" component manually for now.
        
        Handle<Entity>  Create(Handle<Entity> parent = Null<Entity>(), const char* pDebugName = nullptr);
        void            Remove(Handle<Entity> entity);
        void            GetEntities(nv::Vector<Handle<Entity>>& outEntities) const;
        constexpr Span<Entity> GetEntitySpan() const { return mEntities.Span(); }
        constexpr Handle<Entity> GetRootEntity() const { return mRoot; }

        constexpr Entity* GetEntity(Handle<Entity> handle) const { return mEntities.Get(handle); }
        constexpr std::unordered_map<std::string, Handle<Entity>>& GetEntityNameMap() { return mEntityNames; }

        void Serialize(std::ostream& o);
        void Deserialize(std::istream& i);

    private:
        Pool<Entity>            mEntities;
        Handle<Entity>          mRoot;

        UnorderedMap<std::string, Handle<Entity>> mEntityNames;
    };

    extern EntityManager    gEntityManager;

    void SerializeScene(std::ostream& o);
    void DeserializeScene(std::istream& i);

    template<typename TComp>
    EntityComponents<TComp>::Instance EntityComponents<TComp>::operator[](size_t i) const
    {
        return {.mpEntity = gEntityManager.GetEntity(mEntities[i]), .mpComponent = mComponents[i] };
    }
}

#endif // !NV_ENGINE_ENTITYCOMPONENT
