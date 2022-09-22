#pragma once

#include <Lib/Handle.h>
#include <AssetBase.h>
#include <Engine/Component.h>
#include <Renderer/Texture.h>

namespace nv::graphics::components
{
    enum MaterialType
    {
        MATERIAL_PBR,
        MATERIAL_DIFFUSE
    };

    constexpr uint32_t MAX_MATERIAL_TEXTURES = 4;

    struct PBRMaterial
    {
        using AssetID = asset::AssetID;
        AssetID mAlbedoTexture;
        AssetID mNormalTexture;
        AssetID mRoughnessTexture;
        AssetID mMetalnessTexture;
    };

    struct DiffuseMaterial
    {
        using AssetID = asset::AssetID;
        AssetID mDiffuseTexture;
        AssetID mSpecularTexture;
    };

    struct Material
    {
        MaterialType mType = MATERIAL_PBR;
        Handle<Texture> mTextures[MAX_MATERIAL_TEXTURES];
    };
}