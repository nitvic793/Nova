
#define USE_BLUE_NOISE 1
        
#include "RTCommon.hlsli"

RWTexture2D<float3> OutputTexture : register(u0);
RWTexture2D<float4> DirectGITexture : register(u1);

void GetRayDesc(uint2 px, out RayDesc ray)
{
    uint w, h;
    OutputTexture.GetDimensions(w, h);
    float2 dims = float2(w, h);
    float2 screenPos = (float2(px) + 0.5f)/ dims * 2.0 - 1.0;
    screenPos.y = -screenPos.y;

    float4 world = mul(float4(screenPos, Frame.NearZ, 1), Frame.ViewProjectionInverse);
    world.xyz /= world.w;

    ray.Origin = Frame.CameraPosition;
    ray.TMin = MIN_DIST;
    ray.TMax = MAX_DIST;
    ray.Direction = normalize(world.xyz - ray.Origin);
}

float4 DoInlineRayTracing(RayDesc ray, uint3 DTid)
{
    const float4 HIT_COLOR = float4(1,0,0,1);
    const float4 MISS_COLOR = float4(0,0,0,1);
    float4 result = 0.xxxx;

    uint randSeed = InitRand(DTid.x * Params.FrameCount, DTid.y * Params.FrameCount, 16);
    DefaultRayQueryT rayQuery = ShootRay(ray);

    float3 resultColor = MISS_COLOR.xyz;
	if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
        HitContext ctx;
        uint instanceId = rayQuery.CommittedInstanceID(); // Used to index into array of structs to get data needed to calculate light
        resultColor = OnHit(instanceId, rayQuery, ctx, randSeed, DTid);
	}
	else
    {
        resultColor = OnMiss(ray.Direction, rayQuery);
        return float4(resultColor, 0.f);
    }

    result = float4(resultColor, 1.f);
    return result;
} 

[numthreads(8, 8, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    RayDesc rayDesc;
    GetRayDesc(DTid.xy, rayDesc);
    float4 GIColor = DirectGITexture[DTid.xy];
    OutputTexture[DTid.xy] = DoInlineRayTracing(rayDesc, DTid).xyz + GIColor.xyz;
}