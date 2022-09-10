#pragma once

#ifdef __cplusplus
#include <Math/Math.h>

namespace nv::graphics
{
    using namespace math;
#endif
    struct Vertex
    {
        float3 mPosition;
        float2 mUV;
        float3 mNormal;
        float3 mTangent;
    };

    struct ObjectData
    {
        float4x4 World;
    };

    struct FrameData
    {
        float4x4 View;
        float4x4 Projection;
    };
#ifdef __cplusplus
}
#endif