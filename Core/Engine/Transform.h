#pragma once

#include <Math/Math.h>

namespace nv
{
    // TODO: Convert to Struct of Arrays
    struct Transform
    {
        using float3 = math::float3;
        using float4 = math::float4;
        using float4x4 = math::float4x4;

        float3 mPosition = { 0,0,0 };
        float4 mRotation = { 0,0,0,0 };
        float3 mScale = { 1,1,1 };

        float4x4 GetTransformMatrix() const;
        float4x4 GetTransformMatrixTransposed() const;
    };
}