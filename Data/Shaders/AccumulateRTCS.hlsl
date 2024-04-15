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
    const bool bTemporalReprojection = false;
    if(bTemporalReprojection)
    {
        float depth = DepthTexture[DTid.xy];
        if(depth != 0)
        {

            float2 screenPos = (float2(px) + 0.5f) / float2(dims) * 2.0f - 1.0f;
            float4 worldPos = WorldPosFromDepth(depth, screenPos);

            float4x4 prevViewProjection = mul(Frame.PrevView, Frame.PrevProjection);
            float4 prevWorldPos = mul(float4(worldPos.xyz, 1.0f), prevViewProjection);
            prevWorldPos /= prevWorldPos.w;
            prevWorldPos.y = -prevWorldPos.y;

            float2 pxLastFrameRaw = (prevWorldPos.xy * 0.5f + 0.5f) * float2(dims);
            pxLastFrame = clamp(pxLastFrameRaw, 0.0f, float2(dims - 1));

            if(pxLastFrame.x != pxLastFrameRaw.x || pxLastFrame.y != pxLastFrameRaw.y)
            {
                accumAlpha = 1.0f;
            }
        }
    }

   // if(accumAlpha < 1.0f)
        AccumTex[px] = lerp(PrevAccumTex[px], RawRTTex[px], accumAlpha);
   // else
       // AccumTex[px] = RawRTTex[px];
}