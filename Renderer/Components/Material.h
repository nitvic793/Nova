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

    struct SimpleMaterial
    {
        using float3 = nv::math::float3;

        float3 mDiffuseColor;
        float  mRoughness;
        float  mMetalness;
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
    struct BaseMaterial
    {
        MaterialType mType;
        union
        {
            PBRMaterial     mPBR;
            DiffuseMaterial mDiffuse;
            SimpleMaterial  mSimple;
        } mMat;
    };

    struct MaterialInstance
    {
        enum Offset
        {
            ALBEDO = 0,
            NORMAL,
            ROUGHNESS,
            METALNESS
        };

        MaterialType mType = MATERIAL_PBR;
        Handle<Texture> mTextures[MAX_MATERIAL_TEXTURES];
    };
}