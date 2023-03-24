#ifndef NV_INTEROP_SHADERINTEROPTYPES_H
#define NV_INTEROP_SHADERINTEROPTYPES_H

#pragma once

static const uint32_t MAX_DIRECTIONAL_LIGHTS = 2;

#ifdef __cplusplus
#include <Math/Math.h>
#include <Engine/Component.h>
#define NV_COMPONENT : public nv::ecs::IComponent

namespace nv::graphics
{
    using namespace math;
#else
    #define NV_COMPONENT
#endif
    struct Vertex
    {
        float3 mPosition;
        float2 mUV;
        float3 mNormal;
        float3 mTangent;
    };

    struct VertexPos
    {
        float3 mPosition;
    };

    struct VertexEx
    {
        float2 mUV;
        float3 mNormal;
        float3 mTangent;
    };

    struct ObjectData
    {
        float4x4 World;
        uint32_t MaterialIndex;
        float    _Padding;
    };

    struct MaterialData
    {
        uint32_t AlbedoOffset;
        uint32_t NormalOffset;
        uint32_t RoughnessOffset;
        uint32_t MetalnessOffset;
    };

    struct DirectionalLight NV_COMPONENT
    {
        float3  Direction;
        float   Intensity;
        float3  Color;
        float   _Padding;
    };

    struct PointLight NV_COMPONENT
    {
        float3  Position;
        float   Intensity;
        float3  Color;
        float   Range;
    };

    struct SpotLight NV_COMPONENT
    {
        float3 Color;
        float Intensity;
        float3 Position;
        float Range;
        float3 Direction;
        float SpotFalloff;
    };

    struct FrameData
    {
        float4x4            View;
        float4x4            Projection;
        float4x4            ViewInverse;
        float4x4            ProjectionInverse;
        float4x4            ViewProjectionInverse;
        float3              CameraPosition;
        float               _Padding0;
        DirectionalLight    DirLights[MAX_DIRECTIONAL_LIGHTS];
        uint32_t            DirLightsCount;
        float               _Padding1;
    };

    struct ViewportDesc
    {
        float Left;
        float Top;
        float Right;
        float Bottom;
    };

    struct RayGenConstantBuffer
    {
        ViewportDesc Viewport;
        ViewportDesc Stencil;
    };

    struct HeapState
    {
        uint32_t ConstBufferOffset;
        uint32_t TextureOffset;
    };

    struct TraceParams
    {
        float2 Resolution;
        float  ScaleFactor;
        float  _Padding;
    };

    //BVH Reference: https://jacco.ompf2.com/2022/06/03/how-to-build-a-bvh-part-9a-to-the-gpu/
    struct Intersection
    {
        float T;                    // intersection distance along ray
        float U, V;	                // barycentric coordinates of the intersection
        uint InstPrim;              // instance index (12 bit) and primitive index (20 bit)
    };
    
    struct Ray
    {
        float3          Orig; 
        float3          Dir;
        float3          rDir;            
        Intersection    Hit;           
    };

#ifdef __cplusplus
}
#endif

#endif // NV_INTEROP_SHADERINTEROPTYPES_H