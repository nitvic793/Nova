#pragma once

#include <span>
#include <Lib/StringHash.h>
#include <vector>
#include <memory>
#include <unordered_map>

namespace nv::sim
{
    template <typename... Types>
    class Data
    {
        static constexpr uint32_t INIT_SIZE = 32;

    public:
        using Instance = std::tuple<Types...>;
        using InstanceRef = std::tuple<Types&...>;

    public:
        void Init()
        {
            Resize(INIT_SIZE);
        }

        void Resize(size_t size)
        {
            std::apply(
                [size](auto&&... args)
                {
                    ((args.resize(size)), ...);
                },
                mItems);
        }

        template<typename T>
        std::vector<T>& Get()
        {
            std::vector<T>& items = std::get<std::vector<T>>(mItems);
            return items;
        }

        template<typename T>
        constexpr std::span<T> GetSpan()
        {
            std::vector<T>& items = std::get<std::vector<T>>(mItems);
            return std::span<T> { items.data(), items.size() };
        }

        Instance GetInstance(size_t idx)
        {
            Instance result;
            std::apply(
                [idx, this, &result](auto&&... args) mutable
                {
                    ((this->Set(idx, args, result)), ...);
                },
                mItems);

            return result;
        }

        InstanceRef GetInstanceRef(size_t idx)
        {
            InstanceRef instRef = { Get<Types>(idx)... };
            return instRef;
        }

    private:
        template<typename T>
        T& Get(size_t idx)
        {
            std::vector<T>& v = Get<T>();
            T& val = v[idx];
            return val;
        }

        template<typename T>
        void Set(size_t idx, T& out)
        {
            out = Get(idx);
        }

        template<typename T>
        void Set(size_t idx, std::vector<T>& v, Instance& instance)
        {
            std::get<T>(instance) = v[idx];
        }

    private:
        std::tuple<std::vector<Types>...> mItems;
    };


    template<typename... Types>
    struct Archetype
    {
        template <template <typename...> typename T>
        using Apply = T<Types...>;

        using Store = Apply<Data>;
    };

    template<typename T>
    using ArchetypeStore = T::Store;

    struct IProperty
    {
        virtual void Resize(size_t size) = 0;
        virtual void Swap(uint32_t idxA, uint32_t idxB) = 0;
        virtual size_t Size() const = 0;
    };

    template<typename T>
    struct Property : public IProperty
    {
        std::vector<T> mItems;

        void Resize(size_t size) override { mItems.resize(size); }
        void Swap(uint32_t idxA, uint32_t idxB) override { std::swap(mItems[idxA], mItems[idxB]); }
        size_t Size() const override { return mItems.size(); }
    };

    class Store
    {
        static constexpr uint32_t INIT_SIZE = 8;
    public:
        template<typename T> void Register(size_t initSize = INIT_SIZE);
        template<typename T> std::span<T> Data();

        void Resize(size_t size);
        void Swap(uint32_t idxA, uint32_t idxB);
        size_t Size() const;

    private:
        std::unordered_map<StringID, std::unique_ptr<IProperty>> mPropertyMap;
    };

    template<typename T>
    inline void Store::Register(size_t initSize)
    {
        constexpr auto typeHash = nv::TypeNameID<T>();
        auto& prop = mPropertyMap[typeHash];
        prop = std::unique_ptr<IProperty>(new Property<T>());
        prop->Resize(initSize);
    }

    template<typename T>
    inline std::span<T> Store::Data()
    {
        constexpr auto typeHash = nv::TypeNameID<T>();
        Property<T>* prop = (Property<T>*)mPropertyMap[typeHash].get();
        return std::span<T>{ prop->mItems.data(), prop->mItems.size() };
    }
}