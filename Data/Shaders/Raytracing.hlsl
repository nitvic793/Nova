#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include "Common.hlsli"

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);
ConstantBuffer<RayGenConstantBuffer> g_rayGenCB : register(b0);
ConstantBuffer<FrameData> g_frameData : register(b1);

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

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, 1);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    payload.color = float4(0, 0, 0, 1);
}

#endif // RAYTRACING_HLSL