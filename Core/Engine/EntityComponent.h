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
        return compName;
    }

    class IComponentPool 
    {
    public:
        IComponentPool(Span<Field> fields) 
        {
            mFields.Reserve(fields.Size());
            for (const Field& field : fields)
                mFields.Push(field);
        }

        virtual bool IsValid(uint32_t index) const = 0;
        virtual bool IsValid(uint64_t handle) const = 0;
        virtual Span<IComponent> GetComponents() const = 0;
        virtual IComponent* GetComponent(uint32_t index) const = 0;
        virtual IComponent* GetComponent(uint64_t handle) const = 0;
        virtual size_t GetComponentSize() const = 0;
        virtual size_t Size() const = 0;

        template<typename TComp>
        IComponent* GetComponent(Handle<TComp> handle)
        {
            if (!IsValid())
                return nullptr;

            return static_cast<TComp*>(GetComponent(handle.mHandle));
        }

    protected:
        nv::Vector<Field>   mFields;
    };

    template<typename TComp>
    class ComponentPool : public IComponentPool
    {
        using EntityComponentMap = HashMap<Handle<Entity>, Handle<TComp>>;

    public:
        ComponentPool(Span<Field> fields):
            IComponentPool(fields)
        {}

        virtual bool IsValid(uint32_t index) const override
        {
            return mComponents.IsValid(index);
        }

        virtual bool IsValid(uint64_t handle) const override
        {
            auto h = Handle<TComp>{ .mHandle = handle };
            return mComponents.IsValid(h);
        }

        virtual Span<IComponent> GetComponents() const override
        {
            nv::Span<TComp> span = mComponents.Span();
            return { (IComponent*)span.mData, span.mSize };
        }

        virtual IComponent* GetComponent(uint32_t index) const override
        {
            return static_cast<IComponent*>(mComponents.GetIndex(index));
        }

        virtual IComponent* GetComponent(uint64_t handle) const override
        {
            auto h = Handle<TComp>{ .mHandle = handle };
            return static_cast<IComponent*>(mComponents.Get(h));
        }

        virtual size_t GetComponentSize() const override
        {
            return sizeof(TComp);
        }

        virtual size_t Size() const override
        {
            return mComponents.Size();
        }

        constexpr Span<TComp> Span() const
        {
            return mComponents.Span();
        }

    private:
        Pool<TComp>         mComponents;
        EntityComponentMap  mEntityCompMap;
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

        template<typename TComp>
        void CreatePool(Span<Field> fields)
        {
            constexpr StringID compId = GetComponentID<TComp>();
            void* buffer = Alloc<ComponentPool<TComp>>(fields);
            mComponentPools[compId] = ScopedPtr<IComponentPool, true>((IComponentPool*)buffer);
        }

    private:
        ComponentPoolMap mComponentPools;
    };

    struct Entity
    {
    public:
        template<typename TComp, typename ...Args>
        void Add(Args&&... args)
        {
            
        }

        template<typename TComp>
        TComp* Get() const
        {
            constexpr StringID compId = GetComponentID<TComp>();
            auto comp = mComponents.at(compId);
            return comp->As<TComp>();
        }

        constexpr void SetHandle(Handle<Entity> handle) { mHandle = handle; }

    public:
        Handle<Entity> mHandle;
        HashMap<StringID, IComponent*> mComponents;
    };

    class EntityManager
    {
    public:
        Handle<Entity>  Create();
        void            Remove(Handle<Entity> entity);

        constexpr Entity* GetEntity(Handle<Entity> handle) const { return mEntities.Get(handle); }
    private:
        Pool<Entity> mEntities;
        ComponentManager mComponentMgr;
    };
}

#endif // !NV_ENGINE_ENTITYCOMPONENT
