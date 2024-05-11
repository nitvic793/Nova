#pragma once

#include <span>
#include <Lib/StringHash.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <Math/Math.h>

namespace nv::sim
{
    template<typename T>
    struct Property
    {
        T mValue = {};

        constexpr operator T() { return mValue; }
        constexpr operator T& () { return mValue; }
        //constexpr operator T&&() { return mValue; }
        constexpr operator T() const { return mValue; }
        constexpr operator const T& () const { return mValue; }

        constexpr T& operator=(T& val) { mValue = val; }
        constexpr T& operator=(T&& val) { mValue = std::move(val); }
        constexpr T& operator=(const T& val) { mValue = val; }

        constexpr Property(const T& val) : mValue(val) {}
        constexpr Property(T&& val) : mValue(val) {}
        constexpr Property() : mValue() {}
    };

    using FloatProperty = Property<float>;
    using Float3Property = Property<math::float3>;
    using UIntProperty = Property<uint32_t>;
    using UInt64Property = Property<uint64_t>;

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

    template<typename... T>
    using TsFunc = void(*)(T...);

    template<typename TProcessor, typename... T>
    using TProcFunc = void(TProcessor::*)(T...);

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

        template<typename... T>
        constexpr void ForEach(TsFunc<T&...> fn, size_t start = 0, size_t end = 0)
        {
            const size_t size = GetSize();
            end = end == 0 ? size : end;

            for (size_t i = start; i < end; ++i)
            {
                fn(Get<T>(i)...);
            }
        }

        template<typename... T, typename TProcessor>
        constexpr void ForEach(TProcFunc<TProcessor, T&...> fn, TProcessor& processor, size_t start = 0, size_t end = 0)
        {
            const size_t size = GetSize();
            end = end == 0 ? size : end;

            for (size_t i = start; i < end; ++i)
            {
                (processor.*fn)(Get<T>(i)...);
            }
        }

        constexpr Instance GetInstance(size_t idx)
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


        constexpr InstRef GetInstanceRef(size_t idx)
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

    template<typename TStore, typename TProcessor>
    class IProcessor
    {
    public:
        constexpr void Invoke(TStore& dataStore)
        {
            dataStore.ForEach(&TProcessor::Process, *(TProcessor*)this);
        }
    };

    class DataStoreFactory
    {
    public:
        template<typename TStore>
        constexpr TStore* GetStore() const
        {
            constexpr StringID typeHash = nv::TypeNameID<TStore>();
            return (TStore*)mStores.at(typeHash).get();
        }

        template<typename TStore>
        constexpr void Register()
        {
            constexpr StringID typeHash = nv::TypeNameID<TStore>();
            mStores[typeHash] = std::unique_ptr<IDataStore>(new TStore());
        }

    private:
        std::unordered_map<StringID, std::unique_ptr<IDataStore>> mStores;
    };

    extern DataStoreFactory sgDataStoreFactory;
}