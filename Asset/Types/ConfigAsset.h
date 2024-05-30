#pragma once

#include <Asset.h>
#include <Configs/MaterialDatabase.h>

#include <ostream>
#include <istream>

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>

namespace cereal
{
    using namespace nv::graphics;
    using namespace nv::asset;

    template<class Archive>
    void save(Archive& archive, MaterialDatabase const& config)
    {
        archive(make_nvp("Materials", gMaterialDatabase.mMaterials));
    }

    template<class Archive>
    void load(Archive& archive, MaterialDatabase& config)
    {
        archive(make_nvp("Materials", config.mMaterials));
        gMaterialDatabase.mMaterials = config.mMaterials;
    }

    template<class Archive>
    void save(Archive& archive, PBRMaterial const& h)
    {
        std::string type = "MaterialBinary";
        archive(make_nvp("Type", type));
        archive(h.mAlbedoTexture.mId);
        archive(h.mNormalTexture.mId);
        archive(h.mRoughnessTexture.mId);
        archive(h.mMetalnessTexture.mId);
    }

    template<class Archive>
    void load(Archive& archive, PBRMaterial& config)
    {
        std::string type;
        archive(make_nvp("Type", type));
        if (type == "MaterialDefinition")
        {
            std::string albedo;
            std::string normal;
            std::string roughness;
            std::string metalness;
            archive(make_nvp("Albedo", albedo));
            archive(make_nvp("Normal", normal));
            archive(make_nvp("Roughness", roughness));
            archive(make_nvp("Metalness", metalness));

            config.mAlbedoTexture = { ASSET_TEXTURE, nv::ID(albedo.c_str()) };
            config.mNormalTexture = { ASSET_TEXTURE, nv::ID(normal.c_str()) };
            config.mRoughnessTexture = { ASSET_TEXTURE, nv::ID(roughness.c_str()) };
            config.mMetalnessTexture = { ASSET_TEXTURE, nv::ID(metalness.c_str()) };
        }
        else
        {
            archive(config.mAlbedoTexture.mId);
            archive(config.mNormalTexture.mId);
            archive(config.mRoughnessTexture.mId);
            archive(config.mMetalnessTexture.mId);
        }
    }
}

namespace nv::asset
{
    using namespace nv::graphics;

    enum SerialType
    {
        SERIAL_JSON,
        SERIAL_BINARY
    };

    template<typename T, SerialType TSerial = SERIAL_JSON>
    void Load(std::istream& i, const char* name)
    {
        T temp;
        if constexpr (TSerial == SERIAL_JSON)
        {
            cereal::JSONInputArchive archive(i);
            archive(cereal::make_nvp(name, temp));
        }
        else if constexpr (TSerial == SERIAL_BINARY)
        {
            cereal::BinaryInputArchive archive(i);
            archive(cereal::make_nvp(name, temp));
        }
    }

    template<typename T, SerialType TSerial = SERIAL_JSON>
    void Export(std::ostream& o, const char* name)
    {
        T temp;
        if constexpr (TSerial == SERIAL_JSON)
        {
            cereal::JSONOutputArchive archive(o);
            archive(cereal::make_nvp(name,temp));
        }
        else if constexpr (TSerial == SERIAL_BINARY)
        {
            cereal::BinaryOutputArchive archive(o);
            archive(cereal::make_nvp(name, temp));
        }
    }
}