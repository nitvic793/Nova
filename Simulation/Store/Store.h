#pragma once

#include <span>
#include <Lib/StringHash.h>
#include <vector>
#include <memory>
#include <unordered_map>

namespace nv::sim
{
    class IDataStore
    {
    public:
        template<typename T>
        constexpr T& As() { return *static_cast<T*>(this); }

        virtual void Init() = 0;
        virtual void Resize(size_t size) = 0;
        virtual size_t GetSize() = 0;
    };

    template <typename... Types>
    struct Ref
    {
        std::tuple<Types&...> mData;

        template<typename T>
        T& Get()
        {
            T& item = std::get<T&>(mData);
            return item;
        }

        template<typename T>
        void Set(const T& val)
        {
            std::get<T&>(mData) = val;
        }

        template<typename T>
        void Set(T&& val)
        {
            std::get<T&>(mData) = val;
        }
    };

    template <typename... Types>
    class DataStore : public IDataStore
    {
        static constexpr uint32_t INIT_SIZE = 32;

    public:
        using Instance = std::tuple<Types...>;
        using InstanceRef = std::tuple<Types&...>;
        using InstRef = Ref<Types...>;

    public:
        void Init() override
        {
            Resize(INIT_SIZE);
        }

        void Resize(size_t size) override
        {
            std::apply(
                [size](auto&&... args)
                {
                    ((args.resize(size)), ...);
                },
                mDataArrays);
        }

        size_t GetSize() override
        {
            auto& v = std::get<0>(mDataArrays);
            return v.size();
        }

        template<typename T>
        std::vector<T>& Get()
        {
            std::vector<T>& items = std::get<std::vector<T>>(mDataArrays);
            return items;
        }

        template<typename T>
        constexpr std::span<T> GetSpan()
        {
            std::vector<T>& items = std::get<std::vector<T>>(mDataArrays);
            return std::span<T> { items.data(), items.size() };
        }

        template<typename... T>
        constexpr std::tuple<std::span<T>...> GetSpans()
        {
            std::tuple<std::span<T>...> spans = { GetSpan<T>()... };
            return spans;
        }

        constexpr std::tuple<std::span<Types>...> GetSpans()
        {
            std::tuple<std::span<Types>...> spans = { GetSpan<Types>()... };
            return spans;
        }

        template<typename... T, typename TFunc>
        constexpr void ForEach(TFunc fn, size_t start = 0, size_t end = 0)
        {
            const size_t size = GetSize();
            end = end == 0 ? size : end;

            for (size_t i = start; i < end; ++i)
            {
                fn(Get<T>(i)...);
            }
        }

        Instance GetInstance(size_t idx)
        {
            Instance result;
            std::apply(
                [idx, this, &result](auto&&... args) mutable
                {
                    ((this->Set(idx, args, result)), ...);
                },
                mDataArrays);

            return result;
        }


        InstRef GetInstanceRef(size_t idx)
        {
            InstanceRef instRef = { Get<Types>(idx)... };
            InstRef ref = { instRef };
            return ref;
        }

        template<typename T>
        constexpr T& Get(size_t idx)
        {
            std::vector<T>& v = Get<T>();
            T& val = v[idx];
            return val;
        }

    private:

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
        std::tuple<std::vector<Types>...> mDataArrays;
    };


    template<typename... Types>
    struct Archetype
    {
        template <template <typename...> typename T>
        using Apply = T<Types...>;

        using Store = Apply<DataStore>;
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