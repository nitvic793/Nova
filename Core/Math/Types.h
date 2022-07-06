#ifndef NV_MATH_TYPES
#define NV_MATH_TYPES

#pragma once

#if NV_USE_DIRECTXMATH

#include <DirectXMath.h>

namespace nv
{
    using float2    = DirectX::XMFLOAT2;
    using float3    = DirectX::XMFLOAT3;
    using float4    = DirectX::XMFLOAT4;
    using float3x3  = DirectX::XMFLOAT3X3;
    using float4x3  = DirectX::XMFLOAT4X3;
    using float4x4  = DirectX::XMFLOAT4X4;
}

#endif // NV_USE_DIRECTXMATH

#endif // !NV_MATH_TYPES
