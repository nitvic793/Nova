#pragma once

#include <Lib/Handle.h>
#include <AssetBase.h>
#include <Engine/Component.h>
#include <Renderer/Texture.h>
#include <Math/Math.h>

namespace nv::graphics
{
    enum MaterialType
    {
        MATERIAL_PBR,
        MATERIAL_DIFFUSE,
        MATERIAL_SIMPLE
    };

    constexpr uint32_t MAX_MATERIAL_TEXTURES = 4;
    constexpr float DEFAULT_ROUGHNESS = 0.1f;
    constexpr float DEFAULT_METALNESS = 0.01f;
    constexpr math::float3 DEFAULT_DIFFUSE = { 0.f, 0.f, 0.f };

    struct SimpleMaterial
    {
        using float3 = nv::math::float3;

        float3 mDiffuseColor    = DEFAULT_DIFFUSE;
        float  mRoughness       = DEFAULT_ROUGHNESS;
        float  mMetalness       = DEFAULT_METALNESS;
        float3 _Padding;
    };

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

    // TODO : Material Database should use this
    struct Material
    {
        MaterialType mType;
        union
        {
            PBRMaterial     mPBR;
            DiffuseMaterial mDiffuse;
            SimpleMaterial  mSimple;
        };
    };

    struct MaterialInstance
    {
        enum PBROffset
        {
            ALBEDO = 0,
            NORMAL,
            ROUGHNESS,
            METALNESS
        };

        MaterialType mType = MATERIAL_PBR;
        union
        {
            Handle<Texture> mTextures[MAX_MATERIAL_TEXTURES];
        };
    };
}