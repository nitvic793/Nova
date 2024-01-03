#pragma once

#include <Math/Math.h>
#include <Engine/Component.h>

namespace nv
{
    enum CameraType : uint8_t
    {
        CAMERA_PERSPECTIVE = 0,
        CAMERA_ORTHOGRAPHIC
    };

    struct CameraDesc
    {
        float       mWidth;
        float       mHeight;
        float       mNearZ = 0.1f;
        float       mFarZ = 1000.f;
        float       mFovAngle = 60.f; 
        CameraType  mType = CAMERA_PERSPECTIVE;


        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(mWidth, mHeight, mNearZ, mFarZ, mFovAngle, mType);
        }
    };

    class Camera
    {
        using float3 = math::float3;
        using float4 = math::float4;
        using float4x4 = math::float4x4;
    public:
        Camera(const CameraDesc& desc);
        Camera() {}

        void UpdateViewProjection();

        void SetParams(const CameraDesc& desc);
        void SetPosition(const float3& position);
        void SetRotation(const float3& rotation);

        float4x4 GetView() const;
        float4x4 GetProjection() const;
        float4x4 GetViewTransposed() const;
        float4x4 GetViewInverseTransposed() const;
        float4x4 GetProjTransposed() const;
        float4x4 GetProjInverseTransposed() const;
        float4x4 GetViewProjInverseTransposed() const;

        const float3&       GetPosition()  const { return mPosition; }
        const float3&       GetDirection() const { return mDirection; }
        const float3&       GetRotation()  const { return mRotation; }
        const CameraDesc&   GetDesc()      const { return mDesc; }
        constexpr float     GetNearZ()     const { return mDesc.mNearZ; }
        constexpr float     GetFarZ()      const { return mDesc.mFarZ; }

    private:
        float3      mPosition;
        float3      mDirection;
        float3      mRotation;
        float4x4    mProjection;
        float4x4    mView;

        CameraDesc  mDesc;

        friend struct CameraComponent;
    };

    struct CameraComponent : public ecs::IComponent
    {
        Camera mCamera;

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(mCamera.mPosition);
            archive(mCamera.mDirection);
            archive(mCamera.mRotation);
            archive(mCamera.mProjection);
            archive(mCamera.mView);
        }
    };
}