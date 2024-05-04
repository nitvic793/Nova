#pragma once

#include <span>
#include <Lib/StringHash.h>
#include <vector>
#include <memory>
#include <unordered_map>

namespace nv::sim
{
    struct IProperty
    {
        virtual void Resize(size_t size) = 0;
    };

    template<typename T>
    struct Property : public IProperty
    {
        std::vector<T> mItems;

        void Resize(size_t size) override { mItems.resize(size); }
    };

    class Store
    {
        static constexpr uint32_t INIT_SIZE = 8;
    public:
        template<typename T> void Register(size_t initSize = INIT_SIZE);
        template<typename T> std::span<T> Data();

        void Resize(size_t size);
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

    void RegisterStores();
}