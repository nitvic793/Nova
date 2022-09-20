#ifndef NV_ENGINE_ENTITYCOMPONENT
#define NV_ENGINE_ENTITYCOMPONENT

#pragma once

#include <Lib/Handle.h>
#include <Lib/Vector.h>
#include <Lib/Pool.h>
#include <Lib/Map.h>
#include <Lib/ScopedPtr.h>

namespace nv::ecs
{
    struct Entity;
    struct IComponent 
    {
        template<typename T>
        T* As()
        {
            return static_cast<T*>(this);
        }
    };

    enum FieldType : uint8_t
    {
        FIELD_UNDEFINED = 0,
        FIELD_FLOAT,
        FIELD_INT,
        FIELD_FLOAT2,
        FIELD_FLOAT3,
        FIELD_FLOAT4,
        FIELD_STRING
    };

    struct Field
    {
        std::string mFieldName;
        FieldType   mType;
        uint32_t    mOffset;
    };

    template<typename TComp>
    constexpr StringID GetComponentID()
    {
        std::string_view compName = TypeName<TComp>();
        constexpr std::string_view prefix = "struct ";
        compName.remove_prefix(prefix.size());
        return ID(compName.data());
    }

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

        template<typename TComp>
        IComponent* GetComponent(Handle<TComp> handle)
        {
            if (!IsValid(handle.mHandle))
                return nullptr;

            return static_cast<TComp*>(GetComponent(handle.mHandle));
        }

    protected:
        nv::Vector<Field>   mFields;
    };

    template<typename TComp>
    class ComponentPool : public IComponentPool
    {
        using EntityComponentMap = HashMap<uint64_t, Handle<TComp>>;

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
            return static_cast<IComponent*>(&mComponents[index]);
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

    private:
        ContiguousPool<TComp>   mComponents;
        EntityComponentMap      mEntityMap;

        friend class ComponentManager;
    };

    class ComponentManager
    {
        using ComponentPoolMap = HashMap<StringID, ScopedPtr<IComponentPool, true>>;

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
            auto& pool = mComponentPools.at(compId);
            ComponentPool<TComp>* cPool = pool.As<ComponentPool<TComp>>();
            return cPool->mComponents.Span();
        }

        template<typename TComp>
        void CreatePool(Span<Field> fields)
        {
            constexpr StringID compId = GetComponentID<TComp>();
            void* buffer = Alloc<ComponentPool<TComp>>(SystemAllocator::gPtr, fields);
            mComponentPools[compId] = ScopedPtr<IComponentPool, true>((IComponentPool*)buffer);
        }

        bool IsPoolAvailable(StringID id) const
        {
            return mComponentPools.find(id) != mComponentPools.end();
        }

    private:
        ComponentPoolMap mComponentPools;
    };

    extern ComponentManager gComponentManager;

    struct Entity
    {
    public:
        template<typename TComp, typename ...Args>
        TComp* Add(Args&&... args)
        {
            constexpr StringID compId = GetComponentID<TComp>();
            if (!gComponentManager.IsPoolAvailable(GetComponentID<TComp>()))
                gComponentManager.CreatePool<TComp>({ nullptr, 0 });
            
            ComponentPool<TComp>* pool = gComponentManager.GetPool<TComp>();
            auto handle = pool->Create(mHandle);
            mComponents[compId] = handle;
            return pool->GetComponent(handle)->As<TComp>();
        }

        template<typename TComp>
        TComp* Get() const
        {
            constexpr StringID compId = GetComponentID<TComp>();
            auto comp = mComponents.at(compId);
            ComponentPool<TComp>* pool = gComponentManager.GetPool<TComp>();
            return pool->GetComponent(comp)->As<TComp>();
        }

        constexpr void SetHandle(Handle<Entity> handle) { mHandle = handle; }

    public:
        Handle<Entity> mHandle;
        HashMap<StringID, uint64_t> mComponents;
    };

    class EntityManager
    {
    public:
        Handle<Entity>  Create();
        void            Remove(Handle<Entity> entity);

        constexpr Entity* GetEntity(Handle<Entity> handle) const { return mEntities.Get(handle); }
    private:
        ContiguousPool<Entity>  mEntities;
        ComponentManager        mComponentMgr;
    };

    extern EntityManager    gEntityManager;
}

#endif // !NV_ENGINE_ENTITYCOMPONENT
