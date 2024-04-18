
#include "RTCommon.hlsli"

RWTexture2D<float3> OutputTexture : register(u0);

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
        ShadeContext shadeCtx;
        uint instanceId = rayQuery.CommittedInstanceID(); // Used to index into array of structs to get data needed to calculate light
        /*resultColor =*/ OnHitGGX(instanceId, rayQuery, ctx, randSeed, DTid, shadeCtx);
        const bool bEnableIndirectGI = Params.EnableIndirectGI; // Diffuse GI
        const bool bCosSampling = true;
#define USE_GGX 1
#if USE_GGX
        if (bEnableIndirectGI)
        {
            float3 V = normalize(Frame.CameraPosition - shadeCtx.WorldPos);
            resultColor += ggxIndirect(randSeed, shadeCtx.WorldPos, shadeCtx.Normal, shadeCtx.Normal, V, shadeCtx.Color, shadeCtx.Metallic, shadeCtx.Roughness, 0, DTid);
        }
#else
        if (bEnableIndirectGI)
        {
            float3 bounceDir;
            if (bCosSampling)
            {
                bounceDir = GetCosHemisphereSample(randSeed, ctx.WorldNormal.xyz);
            }
            else
            {
                bounceDir = GetUniformHemisphereSample(randSeed, ctx.WorldNormal.xyz);
            }
            float NdotL = saturate(dot(ctx.WorldNormal.xyz, bounceDir));
            float3 bounceColor = ShootIndirectRay(ctx.WorldPos, bounceDir, MIN_DIST, randSeed, DTid);
            float sampleProb = bCosSampling ? NdotL / PI : 1.0f / (2.0f * PI);

            resultColor += (NdotL * bounceColor * ctx.Color / PI) / sampleProb;
        }
#endif
    }
	else
    {
        resultColor = float3(0, 0, 0); //OnMiss(ray.Direction, rayQuery);
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
    OutputTexture[DTid.xy] = DoInlineRayTracing(rayDesc, DTid).xyz;
}