#pragma once

#include <Components/Material.h>
#include <unordered_map>

namespace nv::asset
{
    using namespace nv::graphics;

    struct MaterialDatabase
    {
        std::unordered_map<std::string, PBRMaterial> mMaterials; // TODO: Value should be BaseMaterial union where material type is defined by MaterialType enum
    };

    extern MaterialDatabase gMaterialDatabase;
}