#include "Common.hlsli"

ConstantBuffer<TraceAccumParams> Params     : register(b0);
ConstantBuffer<FrameData>   Frame           : register(b1);
Texture2D<float>            DepthTexture    : register(t0);
RWTexture2D<float3>         RawRTTex        : register(u0);

float4 WorldPosFromDepth(float depth, float2 uv)
{
    float z = depth;// * 2.0 - 1.0;
    float x = uv.x * 2.f - 1.f;
    float y = (1.f - uv.y) * 2.f - 1.f;

    float4 clipSpacePosition = float4(x, y, z, 1.0);
    float4 viewSpacePosition = mul(clipSpacePosition, Frame.ProjectionInverse);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
    float4 worldSpacePosition = mul(viewSpacePosition, Frame.ViewInverse);

    return worldSpacePosition;
}


[numthreads(8, 8, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    uint2 dims = uint2(1920, 1080);
    uint2 px = DTid.xy;
    RWTexture2D<float3> AccumTex = ResourceDescriptorHeap[Params.AccumulationTexIdx];
    RWTexture2D<float3> PrevAccumTex = ResourceDescriptorHeap[Params.PrevFrameTexIdx];
    float accumAlpha = (Params.FrameIndex == 0) ? 1.0f : Params.AccumulationAlpha;
    float2 pxLastFrame = float2(px);
    const bool bTemporalReprojection = true;
    if(bTemporalReprojection)
    {
        float depth = DepthTexture[DTid.xy];
        if(depth != 0)
        {
            // Temporal Reprojection
            float4 worldPos = WorldPosFromDepth(depth, DTid.xy / float2(dims));

            float4x4 prevViewProjection = mul(Frame.PrevView, Frame.PrevProjection);
            float4 prevWorldPos = mul(float4(worldPos.xyz, 1.0f), prevViewProjection);
            prevWorldPos /= prevWorldPos.w;
            prevWorldPos.y = -prevWorldPos.y;
            
            worldPos /= worldPos.w;
            worldPos.y = -worldPos.y;
            float2 uvLastFrame = worldPos.xy * 0.5f + 0.5f;
            pxLastFrame = uvLastFrame * float2(dims);

            float2 pxLastFrameRaw = (prevWorldPos.xy * 0.5f + 0.5f) * float2(dims);
            pxLastFrame = clamp(pxLastFrameRaw, 0.0f, float2(dims - 1));

            if (pxLastFrame.x != pxLastFrameRaw.x || pxLastFrame.y != pxLastFrameRaw.y)
            {
                accumAlpha = 1.0f;
            }
            
        }
    }
    
    float3 accumLastFrame = PrevAccumTex[pxLastFrame].xyz;
    float3 raw = RawRTTex[px].xyz;

	// Neighborhood clamp
    bool bHistoryClamp = true;
    if (bHistoryClamp)
    {
        float colorMin = raw;
        float colorMax = raw;

        for (int i = -1; i <= 1; ++i)
        {
            for (int j = -1; j <= 1; ++j)
            {
                int2 readPx = clamp(int2(px) + int2(i, j), 0, int2(dims) - int2(1, 1));
                float3 color = RawRTTex[readPx].xyz;
                colorMin = min(colorMin, color);
                colorMax = max(colorMax, color);
            }
        }

        accumLastFrame = clamp(accumLastFrame, colorMin, colorMax);
    }


    if(accumAlpha < 1.0f)
        AccumTex[px] = lerp(accumLastFrame, RawRTTex[px], accumAlpha);
    else
       AccumTex[px] = RawRTTex[px];
}