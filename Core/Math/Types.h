#ifndef NV_MATH_TYPES
#define NV_MATH_TYPES

#ifdef __cplusplus

#pragma once

#if NV_USE_DIRECTXMATH

#include <DirectXMath.h>

namespace nv::math
{
    using float2    = DirectX::XMFLOAT2;
    using float3    = DirectX::XMFLOAT3;
    using float4    = DirectX::XMFLOAT4;
    using float3x3  = DirectX::XMFLOAT3X3;
    using float4x3  = DirectX::XMFLOAT4X3;
    using float4x4  = DirectX::XMFLOAT4X4;
    using uint      = uint32_t;
    using uint2     = DirectX::XMUINT2;
    using uint3     = DirectX::XMUINT3;
    using uint4     = DirectX::XMUINT4;
    using int2      = DirectX::XMINT2;
    using int3      = DirectX::XMINT3;
    using int4      = DirectX::XMINT4;

    using Vector = DirectX::XMVECTOR;
    using Matrix = DirectX::XMMATRIX;
}

#endif // NV_USE_DIRECTXMATH

#endif

#endif // !NV_MATH_TYPES
