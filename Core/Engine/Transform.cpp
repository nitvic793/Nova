#include "pch.h"
#include "Transform.h"

namespace nv
{ 
    using namespace math;

    float4x4 Transform::GetTransformMatrix() const
    {
		auto pos = Load(mPosition);
		auto rot = Load(mRotation);
		auto scaleV = Load(mScale);
		
		auto translation = MatrixTranslation(pos);
		auto rotation = MatrixRotationQuaternion(rot);
		auto scale = MatrixScaling(scaleV);
		auto transformation = scale * rotation * translation;

		float4x4 transformMatrix;
		Store(transformation, transformMatrix);
		return transformMatrix;
    }

    float4x4 Transform::GetTransformMatrixTransposed() const
    {
		float4x4 mat = GetTransformMatrix();
		Store(MatrixTranspose(Load(mat)), mat);
        return mat;
    }

	float4x4 Transform::GetTransformMatrixInverseTransposed() const
	{
        float4x4 mat = GetTransformMatrix();
		auto mmat = Load(mat);
		mmat = MatrixInverse(mmat);
        Store(MatrixTranspose(mmat), mat);
        return mat;
	}

	float4x4 TransformRef::GetTransformMatrix() const
	{
		auto pos = Load(mPosition);
		auto rot = Load(mRotation);
		auto scaleV = Load(mScale);

		auto translation = MatrixTranslation(pos);
		auto rotation = MatrixRotationQuaternion(rot);
		auto scale = MatrixScaling(scaleV);
		auto transformation = scale * rotation * translation;

		float4x4 transformMatrix;
		Store(transformation, transformMatrix);
		return transformMatrix;
	}

	float4x4 TransformRef::GetTransformMatrixTransposed() const
	{
		float4x4 mat = GetTransformMatrix();
		Store(MatrixTranspose(Load(mat)), mat);
		return mat;
	}

	float4x4 TransformRef::GetTransformMatrixInverseTransposed() const
	{
        float4x4 mat = GetTransformMatrix();
        auto mmat = Load(mat);
        mmat = MatrixInverse(mmat);
        Store(MatrixTranspose(mmat), mat);
        return mat;
	}

}
