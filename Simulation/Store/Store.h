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

        constexpr T& operator=(T& val) { mValue = val; return mValue; }
        constexpr T& operator=(T&& val) { mValue = std::move(val); return mValue; }
        constexpr T& operator=(const T& val) { mValue = val; return mValue;}

        constexpr Property(const T& val) : mValue(val) {}
        constexpr Property(T&& val) : mValue(val) {}
        constexpr Property() : mValue() {}

        template<typename Archive> void serialize(Archive& archive) { archive(mValue); }
    };

    struct StoreIndex
    {
        size_t mIndex = 0;
    };

    using FloatProperty     = Property<float>;
    using Float3Property    = Property<math::float3>;
    using UIntProperty      = Property<uint32_t>;
    using UInt64Property    = Property<uint64_t>;

    class IDataStore
    {
    public:
        template<typename T>
        constexpr T& As() { return *static_cast<T*>(this); }

        virtual void Init() = 0;
        virtual void Resize(size_t size) = 0;
        virtual size_t GetSize() = 0;
    };

    class BaseProcessor 
    {
    public:
        virtual void Invoke(IDataStore* dataStore, size_t start = 0, size_t end = 0) = 0;
    };

    class BaseBatchProcessor
    {
    public:
        virtual void Invoke(IDataStore* dataStore) = 0;
    };

    template<typename TStore, typename TProcessor>
    class IProcessor : public BaseProcessor
    {
    public:
        void Invoke(IDataStore* dataStore, size_t start = 0, size_t end = 0) override
        {
            Invoke(*(TStore*)dataStore, start, end);
        }

        constexpr void Invoke(TStore& dataStore, size_t start = 0, size_t end = 0)
        {
            dataStore.ForEach(&TProcessor::Process, *(TProcessor*)this);
        }
    };

    template<typename TStore, typename TProcessor>
    class IBatchProcessor : public BaseBatchProcessor
    {
    public:
        constexpr void Invoke(IDataStore* dataStore) override
        {
            Invoke(*(TStore*)dataStore);
        }

        constexpr void Invoke(TStore& dataStore)
        {
            dataStore.OnAll(&TProcessor::Process, *(TProcessor*)this);
        }
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
    public:
        using Instance = std::tuple<Types...>;
        using InstanceRef = std::tuple<Types&...>;
        using InstRef = Ref<Types...>;

        template<int N>
        using NthType = std::tuple_element<N, Instance>::type;
        using IndexType = NthType<0>;

    public:
        void Init() override {}

        void Resize(size_t size) override
        {
            std::apply(
                [size](auto&&... args)
                {
                    ((args.resize(size)), ...);
                },
                mDataArrays);
        }

        template<typename UUIDFn>
        constexpr InstRef Emplace(UUIDFn uuidFn)
        {
            const auto uuid = uuidFn();
            const auto idx = Get<IndexType>().size();

            std::apply(
                [uuid](auto&&... args)
                {
                    ((args.emplace_back()), ...);
                },
                mDataArrays);

            auto instRef = GetInstanceRef(idx);

            return instRef;
        }

        size_t GetSize() override
        {
            auto& v = std::get<0>(mDataArrays);
            return v.size();
        }

        template<typename T>
        constexpr std::vector<T>& Get()
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

        template<typename... T, typename TProcessor>
        constexpr void OnAll(TsFunc<std::span<T>...> fn, TProcessor& processor)
        {
            (processor.*fn)(GetSpan<T>()...);
        }

        template<typename... T, typename TProcessor>
        constexpr void OnAll(TProcFunc<TProcessor, std::span<T>...> fn, TProcessor& processor)
        {
            (processor.*fn)(GetSpan<T>()...);
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

        constexpr InstRef Find(IndexType key)
        {
            return GetInstanceRef(key.mIndex);
        }

        constexpr void PopBack()
        {
            std::apply(
                [](auto&&... args)
                {
                    ((args.pop_back()), ...);
                },
                mDataArrays);
        }

        constexpr void Swap(size_t idxA, size_t idxB)
        {
            std::apply(
                [idxA, idxB](auto&&... args)
                {
                    ((std::swap(args[idxA], args[idxB])), ...);
                },
                mDataArrays);
        }

        template<typename T>
        constexpr T& Get(size_t idx)
        {
            std::vector<T>& v = Get<T>();
            T& val = v[idx];
            return val;
        }

        template<typename TProcessor>
        constexpr void RegisterProcessor()
        {
            constexpr StringID typeHash = nv::TypeNameID<TProcessor>();
            mProcessors[typeHash] = std::unique_ptr<BaseProcessor>(new TProcessor());
        }

        template<typename TBatchProcessor>
        constexpr void RegisterBatchProcessor()
        {
            constexpr StringID typeHash = nv::TypeNameID<TBatchProcessor>();
            mBatchProcessors[typeHash] = std::unique_ptr<BaseBatchProcessor>(new TBatchProcessor());
        }

        template<typename TProcessor>
        constexpr TProcessor* GetProcessor() const
        {
            constexpr StringID typeHash = nv::TypeNameID<TProcessor>();
            return (TProcessor * )mProcessors.at(typeHash).get();
        }

        template<typename TProcessor>
        constexpr TProcessor* GetBatchProcessor() const
        {
            constexpr StringID typeHash = nv::TypeNameID<TProcessor>();
            return (TProcessor*)mBatchProcessors.at(typeHash).get();
        }

        void Tick()
        {
            for (auto& processor : mProcessors)
                processor.second->Invoke(this);

            for (auto& processor : mBatchProcessors)
                processor.second->Invoke(this);
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
        std::unordered_map<StringID, std::unique_ptr<BaseProcessor>> mProcessors;
        std::unordered_map<StringID, std::unique_ptr<BaseBatchProcessor>> mBatchProcessors;
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