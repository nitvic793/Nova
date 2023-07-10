#pragma once

#include <Lib/Handle.h>
#include <Renderer/Mesh.h>
#include <Engine/Component.h>
#include <Components/Material.h>
#include <Types/Serializers.h>
#include <AssetBase.h>

namespace nv::graphics::components
{
    enum RenderableFlags : uint32_t
    {
        RENDERABLE_FLAG_NONE        = 0,
        RENDERABLE_FLAG_STATIC      = 1,
        RENDERABLE_FLAG_DYNAMIC     = 1 << 1,
        RENDERABLE_FLAG_ANIMATED    = 1 << 2
    };

    struct Renderable : public ecs::IComponent
    {
        Handle<Mesh>        mMesh       = Null<Mesh>();
        Handle<Material>    mMaterial   = Null<Material>();
        RenderableFlags     mFlags      = RENDERABLE_FLAG_NONE;

        constexpr bool HasFlag(RenderableFlags flag) const
        {
            return (mFlags & flag) != 0;
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(mMesh);
            archive(mMaterial);
            archive(mFlags);
        }
    };

    struct AnimationComponent : public ecs::IComponent
    {
        uint32_t			mCurrentAnimationIndex = 0;
        float				mTotalTime = 0.f;
        float				mAnimationSpeed = 1.f;
        bool				mIsPlaying = false;

        NV_SERIALIZE(mCurrentAnimationIndex, mTotalTime, mAnimationSpeed, mIsPlaying);
    };


    struct SkyboxComponent : public ecs::IComponent
    {
        Handle<Texture> mSkybox = Null<Texture>();

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(mSkybox);
        }
    };
}