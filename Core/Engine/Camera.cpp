#include "pch.h"
#include "Camera.h"

namespace nv
{
    using namespace math;

    math::Vector CalcDirection(const float3& rotation, const float3& direction)
    {
        math::Vector rotationVec = math::QuaternionRotationRollPitchYawFromVector(Load(rotation));
        auto dirVec = VectorSet(0, 0, 1, 0);
        dirVec = Vector3Rotate(dirVec, rotationVec);
        return dirVec;
    }

    Camera::Camera(const CameraDesc& desc):
        mDesc(desc),
        mPosition(0,0,0.5f),
        mDirection(0,0,1.f),
        mRotation(0,0,0)
    {
        SetParams(desc);
    }

    void Camera::SetParams(const CameraDesc& desc)
    {
        mDesc = desc;
        UpdateViewProjection();
    }

    void Camera::UpdateViewProjection()
    {
        constexpr float MIN_NEAR_Z = 0.001f;
        constexpr float3 UP_DIR = float3(0, 1, 0);

        const float nearZ = std::max(mDesc.mNearZ, MIN_NEAR_Z);
        const float farZ = std::max(mDesc.mFarZ, nearZ);
        const float aspectRatio = mDesc.mWidth / mDesc.mHeight;
        const float fov = (mDesc.mFovAngle / 180.f) * math::PI;

        auto dir = CalcDirection(mRotation, mDirection);
        auto view = MatrixLookToLH(Load(mPosition), dir, Load(UP_DIR));
        Store(dir, mDirection);
        Store(view, mView);

        Matrix projection = {};
        switch (mDesc.mType)
        {
        case CAMERA_ORTHOGRAPHIC:
            // TODO
        case CAMERA_PERSPECTIVE:
            projection = MatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
        }

        Store(projection, mProjection);
    }

    void Camera::SetPosition(const float3& position)
    {
        mPosition = position;
    }

    void Camera::SetRotation(const float3& rotation)
    {
        mRotation = rotation;
    }

    float4x4 Camera::GetView() const
    {
        return mView;
    }

    float4x4 Camera::GetProjection() const
    {
        return mProjection;
    }

    float4x4 Camera::GetViewTransposed() const
    {
        float4x4 viewT;

        auto view = MatrixTranspose(Load(mView));
        Store(view, viewT);

        return viewT;
    }

    float4x4 Camera::GetViewInverseTransposed() const
    {
        float4x4 viewT;
        auto viewInverse = MatrixTranspose(MatrixInverse(Load(mView)));
        Store(viewInverse, viewT);
        return viewT;
    }

    float4x4 Camera::GetProjTransposed() const
    {
        float4x4 projT;

        auto proj = MatrixTranspose(Load(mProjection));
        Store(proj, projT);

        return projT;
    }

    float4x4 Camera::GetProjInverseTransposed() const
    {
        float4x4 projT;
        auto projInv = MatrixTranspose(MatrixInverse(Load(mProjection)));
        Store(projInv, projT);
        return projT;
    }

    float4x4 Camera::GetViewProjInverseTransposed() const
    {
        float4x4 viewProjT;
        auto viewProj = Load(mView) * Load(mProjection);
        auto viewProjInv = MatrixTranspose(MatrixInverse(viewProj));
        Store(viewProjInv, viewProjT);
        return viewProjT;
    }
}
