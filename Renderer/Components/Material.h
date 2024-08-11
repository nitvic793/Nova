#pragma once

#include <Lib/Handle.h>
#include <AssetBase.h>
#include <Engine/Component.h>
#include <Renderer/Texture.h>
#include <Math/Math.h>

namespace nv::graphics
{
    enum MaterialType : uint8_t
    {
        MATERIAL_PBR,
        MATERIAL_DIFFUSE,
        MATERIAL_SIMPLE
    };

    constexpr uint32_t MAX_MATERIAL_TEXTURES = 4;
    constexpr float DEFAULT_ROUGHNESS = 0.1f;
    constexpr float DEFAULT_METALNESS = 0.01f;
    constexpr math::float3 DEFAULT_DIFFUSE = { 0.f, 0.f, 0.f };
    constexpr math::float3 DEFAULT_EMISSIVE = { 0.f, 0.f, 0.f };
    constexpr asset::AssetID INVALID_TEXTURE = { asset::ASSET_TEXTURE, RES_ID_NULL };

    struct TextureBlob
    {
        const uint8_t* mpData;
        const size_t   mSize;
    };

    struct SimpleMaterial
    {
        using float3 = nv::math::float3;

        float3 mDiffuseColor    = DEFAULT_DIFFUSE;
        float  mRoughness       = DEFAULT_ROUGHNESS;
        float  mMetalness       = DEFAULT_METALNESS;
        float3 mEmissive        = DEFAULT_EMISSIVE;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(mDiffuseColor);
            archive(mRoughness);
            archive(mMetalness); 
            archive(mEmissive);
        }
    };

    struct PBRMaterial
    {
        using AssetID = asset::AssetID;
        AssetID mAlbedoTexture;
        AssetID mNormalTexture;
        AssetID mRoughnessTexture;
        AssetID mMetalnessTexture;
    };

    struct MaterialPBR
    {
        using AssetID = asset::AssetID;
        AssetID mAlbedoTexture    = INVALID_TEXTURE;
        AssetID mNormalTexture    = INVALID_TEXTURE;
        AssetID mRoughnessTexture = INVALID_TEXTURE;
        AssetID mMetalnessTexture = INVALID_TEXTURE;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(mAlbedoTexture.mId);
            archive(mNormalTexture.mId);
            archive(mRoughnessTexture.mId);
            archive(mMetalnessTexture.mId);
        }
    };

    struct DiffuseMaterial
    {
        using AssetID = asset::AssetID;
        AssetID mDiffuseTexture;
        AssetID mSpecularTexture;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(mDiffuseTexture.mId);
            archive(mSpecularTexture.mId);
        }
    };

    // TODO : Material Database should use this
    struct Material
    {
        MaterialType mType;
        union
        {
            MaterialPBR     mPBR;
            DiffuseMaterial mDiffuse;
            SimpleMaterial  mSimple;
        } mData;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(mType);
            switch (mType)
            {
            case MATERIAL_DIFFUSE:  archive(mData.mDiffuse); return;
            case MATERIAL_PBR    :  archive(mData.mPBR);     return;
            case MATERIAL_SIMPLE :  archive(mData.mSimple);  return;
            }
        }
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
            SimpleMaterial  mSimple;
        };
    };
}