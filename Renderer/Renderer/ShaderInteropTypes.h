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
        float3 mUV;
        float3 mNormal;
        float3 mTangent;
    };
#ifdef __cplusplus
}
#endif