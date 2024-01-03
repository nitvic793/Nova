#ifndef NV_INTEROP_SHADERINTEROPTYPES_H
#define NV_INTEROP_SHADERINTEROPTYPES_H

#pragma once

static const uint32_t MAX_DIRECTIONAL_LIGHTS = 2;
static const uint32_t MAX_BONES = 128;

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

        #ifdef __cplusplus
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(Direction, Intensity, Color);
        }
        #endif
    };

    struct PointLight NV_COMPONENT
    {
        float3  Position;
        float   Intensity;
        float3  Color;
        float   Range;

        #ifdef __cplusplus
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(Position, Intensity, Color, Range);
        }
        #endif
    };

    struct SpotLight NV_COMPONENT
    {
        float3 Color;
        float Intensity;
        float3 Position;
        float Range;
        float3 Direction;
        float SpotFalloff;

        #ifdef __cplusplus
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(Color, Intensity, Range, Direction, SpotFalloff);
        }
        #endif
    };

    struct FrameData
    {
        float4x4            View;
        float4x4            Projection;
        float4x4            ViewInverse;
        float4x4            ProjectionInverse;
        float4x4            ViewProjectionInverse;
        float3              CameraPosition;
        float               NearZ;
        DirectionalLight    DirLights[MAX_DIRECTIONAL_LIGHTS];
        uint32_t            DirLightsCount;
        float               FarZ;
    };

    struct PerArmature
	{
		float4x4 Bones[MAX_BONES];
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
        uint32_t  StructBufferIdx;
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

    struct BVHNode
    {
        float3      AABBMin;
        uint32_t    LeftFirst;
        float3      AABBMax;
        uint32_t    TriCount;
    };

    struct AABB
    {
        float3 bmin;
        float3 bmax;
    };

    struct Tri
    {
        float3 Vertex0;
        float3 Vertex1;
        float3 Vertex2;
        float3 Centroid;
    };

    struct TriEx
    {
        float2 uv0;
        float2 uv1;
        float2 uv2;
        float3 N0;
        float3 N1;
        float3 N2;
        float dummy;
    };

    struct TLASNode
    {
        float3 aabbMin;
        uint32_t leftRight; // 2x16 bits
        float3 aabbMax;
        uint32_t BLAS;
    };
    
    struct BVHInstance
    {
        float4x4 transform;
        float4x4 invTransform; // inverse transform
        uint32_t idx;
        float3 _padding;
    };

#ifdef __cplusplus
}
#endif

#endif // NV_INTEROP_SHADERINTEROPTYPES_H