#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include "Common.hlsli"
#include "Lighting.hlsli"

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);
ConstantBuffer<RayGenConstantBuffer> g_rayGenCB : register(b0);
ConstantBuffer<FrameData> g_frameData : register(b1);
ConstantBuffer<HeapState> g_heapState : register(b2);

// StructuredBuffer<uint32_t> Indices : register(t0, space1);
// StructuredBuffer<Vertex> Vertices : register(t0, space2);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
    float4 color;
};

bool IsInsideViewport(float2 p, ViewportDesc viewport)
{
    return (p.x >= viewport.Left && p.x <= viewport.Right)
        && (p.y >= viewport.Top && p.y <= viewport.Bottom);
}

inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), g_frameData.ViewProjectionInverse);

    world.xyz /= world.w;
    origin = g_frameData.CameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void MyRaygenShader()
{
    float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

    // Orthographic projection since we're raytracing in screen space.
    // float3 rayDir = float3(0, 0, 1);
    // float3 origin = float3(
    //     lerp(g_rayGenCB.Viewport.Left, g_rayGenCB.Viewport.Right, lerpValues.x),
    //     lerp(g_rayGenCB.Viewport.Top, g_rayGenCB.Viewport.Bottom, lerpValues.y),
    //     0.0f);

    float3 rayDir;
    float3 origin;

    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    //if (IsInsideViewport(origin.xy, g_rayGenCB.Stencil))
    {
        // Trace the ray.
        // Set the ray's extents.
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = rayDir;
        // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
        // TMin should be kept small to prevent missing geometry at close contact areas.
        ray.TMin = 0.001;
        ray.TMax = 10000.0;
        RayPayload payload = { float4(0, 0, 0, 0) };
        TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

        // Write the raytraced color to the output texture.
        RenderTarget[DispatchRaysIndex().xy] = payload.color;
    }
    // else
    // {
    //     // Render interpolated DispatchRaysIndex outside the stencil window
    //     RenderTarget[DispatchRaysIndex().xy] = float4(lerpValues, 0, 1);
    // }
}

float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

template<typename T>
StructuredBuffer<T> GetStructuredBuffer(uint32_t offset)
{
    uint32_t heapOffset = g_heapState.TextureOffset + offset;
    StructuredBuffer<T> buffer = ResourceDescriptorHeap[heapOffset];
    return buffer;
}

uint3 Load3Indices(uint baseIndex, StructuredBuffer<uint32_t> indexBuffer)
{
    uint3 indices;
    indices.x = indexBuffer.Load(baseIndex);
    indices.y = indexBuffer.Load(baseIndex + 1);
    indices.z = indexBuffer.Load(baseIndex + 2);

    return indices;
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    // StructuredBuffer<uint32_t> IndexBuffer = GetStructuredBuffer<uint32_t>(g_rayGenCB.IndexBufferOffset);
    // StructuredBuffer<Vertex> VertexBuffer = GetStructuredBuffer<Vertex>(g_rayGenCB.VertexBufferOffset);

    // float3 hitPosition = HitWorldPosition();

    // // Get the base index of the triangle's first 16 bit index.
    // uint baseIndex = PrimitiveIndex();
    // const uint3 indices = Load3Indices(baseIndex, Indices);
    
    // float3 vertexNormals[3] = { 
    //     Vertices[indices[0]].mNormal, 
    //     Vertices[indices[1]].mNormal, 
    //     Vertices[indices[2]].mNormal 
    // };

    // float3 triangleNormal = HitAttribute(vertexNormals, attr);
    // // //float3 color = 

    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, 1);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    payload.color = float4(0, 0, 0, 1);
}

#endif // RAYTRACING_HLSL