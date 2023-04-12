#include "Common.hlsli"

ConstantBuffer<TraceParams> Params          : register(b0);
ConstantBuffer<FrameData>   Frame           : register(b1);
RWTexture2D<float3>         OutputTexture   : register(u0);
Texture2D<float4>           SkyTexture      : register(t0);

// Define 1/pi
#define INV_PI      0.318309886183790671538
#define INV2PI		0.15915494309189533576888f
#define PI			3.14159265358979323846264f

// Ref: http://cwyman.org/code/dxrTutors/tutors/Tutor10/tutorial10.md.html
// Convert our world space direction to a (u,v) coord in a latitude-longitude spherical map
float2 wsVectorToLatLong(float3 dir)
{
	float3 p = normalize(dir);
	float u = (1.f + atan2(p.x, -p.z) * INV_PI) * 0.5f;
	float v = acos(p.y) * INV_PI;
	return float2(u, v);
}

float3 Trace(Ray ray, uint3 dispatchTId)
{
    float x = Frame.CameraPosition.x;
    float red = dispatchTId.x / (Params.Resolution.x * Params.ScaleFactor);
    float green = dispatchTId.y / (Params.Resolution.y * Params.ScaleFactor);
    return float3(red, green, x);
}

uint2 GetSkyUV( Ray ray, uint2 texDims )
{
    float phi = atan2( ray.Dir.z, ray.Dir.x );
    uint u = (uint)(texDims.x * (phi > 0 ? phi : (phi + 2 * PI)) * INV2PI - 0.5f);
    uint v = (uint)(texDims.y * acos( ray.Dir.y ) * INV_PI - 0.5f);
    return uint2(u, v);
}

inline void GenerateCameraRay(uint2 idx, out Ray ray)
{
    float2 dimensions = (Params.Resolution.xy * Params.ScaleFactor);
    // float2 xy = idx + 0.5f;
    // float2 screenPos = xy / (Params.Resolution.xy * Params.ScaleFactor) * 2.0 - 1.0;
    // screenPos.y = -screenPos.y;

    float2 rayPixelPos = idx;
    float2 ncdXY = (rayPixelPos / dimensions.xy * 0.5f) - 1.0f; // Transfrom [0,1] to [-1, 1]
    ncdXY.y *= -1.0f;

    float4 rayStart = mul(float4(ncdXY, 0.0f, 1.0f), Frame.ViewProjectionInverse); // Near clip plane
    float4 rayEnd = mul(float4(ncdXY, 1.0f, 1.0f), Frame.ViewProjectionInverse); // Far clip plane
    rayStart.xyz /= rayStart.w;
    rayEnd.xyz /= rayEnd.w;
    float3 rayDir = normalize(rayEnd.xyz - rayStart.xyz);

    //float4 world = mul(float4(screenPos, 0, 1), Frame.ViewProjectionInverse);

    ray.Orig = rayStart;
    ray.Dir = rayDir;
    ray.Hit.T = 1e30f; 
}

[numthreads(8, 8, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray;
    GenerateCameraRay(DTid.xy, ray);

    float2 texDims;
    SkyTexture.GetDimensions( texDims.x, texDims.y );

    float2 uv = GetSkyUV(ray, texDims);

    OutputTexture[DTid.xy] = SkyTexture[uv].xyz;//Trace(ray, DTid);
}