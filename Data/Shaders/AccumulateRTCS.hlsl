#include "Common.hlsli"
#include "GBufferCommon.hlsli"

ConstantBuffer<TraceAccumParams> Params     : register(b0);
ConstantBuffer<FrameData>   Frame           : register(b1);
Texture2D<float>            DepthTexture    : register(t0);
RWTexture2D<float3>         RawRTTex        : register(u0);

SamplerState LinearSampler : register(s0);

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

bool NormalDisocclusion(float3 normal, float3 prevNormal)
{
    const float NORMAL_DISTANCE_THRESHOLD = 0.1f;
    float result = pow(abs(dot(normal, prevNormal)), 2.0f);
    if(result > NORMAL_DISTANCE_THRESHOLD)
        return true;

    return false;
}

bool IsReprojectCoordValid(uint2 px, uint2 pxLastFrame, float3 normal, float3 prevNormal, uint2 dims)
{
    bool result = pxLastFrame.x >= 0 && pxLastFrame.y >= 0 && pxLastFrame.x < dims.x && pxLastFrame.y < dims.y;
    result = result && NormalDisocclusion(normal, prevNormal);
    return result;
}

#define ENABLE_ACCUMULATION 1
#define TEMPORAL_FILTER 0

void TemporalFilter(uint2 px, uint2 dims)
{
    RWTexture2D<float3> AccumTex = ResourceDescriptorHeap[Params.AccumulationTexIdx];
    RWTexture2D<float3> PrevAccumTex = ResourceDescriptorHeap[Params.PrevFrameTexIdx];
    RWTexture2D<float>   HistoryLengthTex = ResourceDescriptorHeap[Params.HistoryTexIdx];
    RWTexture2D<float4> PrevNormalsTex = ResourceDescriptorHeap[Params.PrevNormalTexIdx];
    
    
    float3 currentNormals = GetGBuffersNormal(px, Frame);

    float2 uv = px / float2(dims);

    float3 curSample = RawRTTex[px];
    float2 motionVec = GetGBufferMotionVector(px, Frame) * 2.0f - 1.f;
    float2 pxLastFrameUV = uv - motionVec;
    uint2 pxLastFrame = pxLastFrameUV * float2(dims);
    float3 prevNormals = UnpackNormal(PrevNormalsTex[pxLastFrame].xyz);

    float3 output = curSample;
    float historyLength = 1.0f;
    float v = 0.f;
    if(IsReprojectCoordValid(px, pxLastFrame, currentNormals, prevNormals, dims) && Params.FrameIndex != 0)
    {
        v = 1.f;
        float3 prevSample = PrevAccumTex[pxLastFrame];
        historyLength = HistoryLengthTex[pxLastFrame].r + 1.0f;
        float alpha = 1.0f / historyLength;
        output = float3(1,0,0);//lerp(prevSample, curSample, alpha);
    }

    AccumTex[px] = output;
    HistoryLengthTex[px] = historyLength;
}


[numthreads(8, 8, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    uint2 dims = uint2(1920, 1080);
#if TEMPORAL_FILTER
        TemporalFilter(DTid.xy, dims);
#else
    uint2 px = DTid.xy;
    RWTexture2D<float3> AccumTex = ResourceDescriptorHeap[Params.AccumulationTexIdx];
    RWTexture2D<float3> PrevAccumTex = ResourceDescriptorHeap[Params.PrevFrameTexIdx];
    RWTexture2D<float4> PrevNormalsTex = ResourceDescriptorHeap[Params.PrevNormalTexIdx];
    RWTexture2D<uint>   HistoryLengthTex = ResourceDescriptorHeap[Params.HistoryTexIdx];

#if !ENABLE_ACCUMULATION
    AccumTex[px] = RawRTTex[px];
    return;
#endif
    
    float3 prevNormals = UnpackNormal(PrevNormalsTex[px].xyz);
    float3 currentNormals = GetGBuffersNormal(px, Frame);
    
    float accumAlpha = (Params.FrameIndex == 0) ? 1.0f : Params.AccumulationAlpha;
    float2 pxLastFrame = float2(px);
    const bool bTemporalReprojection = false;

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
            float2 uvLastFrame = prevWorldPos.xy * 0.5f + 0.5f;
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

    const bool bVerifyTemporalNormals = true;
    float normalPow = 2.0f;
    if(bVerifyTemporalNormals && accumAlpha < 1.0f)
    {
        float3 normalThisFrame = normalize(currentNormals);
        float3 normalLastFrame = normalize(prevNormals);

        accumAlpha = lerp(1.0f, accumAlpha, pow(clamp(dot(normalThisFrame, normalLastFrame), 0.0f, 1.0f), normalPow));
    }

    if(accumAlpha < 1.0f)
        AccumTex[px] = lerp(accumLastFrame, RawRTTex[px], accumAlpha);
    else
       AccumTex[px] = RawRTTex[px];
#endif
}