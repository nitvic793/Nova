#pragma once

#include <Components/Material.h>
#include <unordered_map>

namespace nv::asset
{
    using namespace nv::graphics;

    struct MaterialDatabase
    {
        std::unordered_map<std::string, PBRMaterial> mMaterials;
    };

    extern MaterialDatabase gMaterialDatabase;
}