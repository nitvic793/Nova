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

float3 ClipAABB(float3 aabb_min, float3 aabb_max, float3 color)
{
    // Note: only clips towards aabb center
    float3 aabb_center = 0.5f * (aabb_max + aabb_min);
    float3 extent_clip = 0.5f * (aabb_max - aabb_min) + 0.001f;

    // Find color vector
    float3 color_vector = color - aabb_center;
    // Transform into clip space
    float3 color_vector_clip = color_vector / extent_clip;
    // Find max absolute component
    color_vector_clip  = abs(color_vector_clip);
    float max_abs_unit = max(max(color_vector_clip.x, color_vector_clip.y), color_vector_clip.z);

    if (max_abs_unit > 1.0)
        return aabb_center + color_vector / max_abs_unit; // clip towards color vector
    else
        return color; // point is inside aabb
}

void NeighbourhoodStdDev(uint2 px, out float3 mean, out float3 stdDev)
{
    float3 m1 = 0.f;
    float3 m2 = 0.f;

    const int radius = 8;
    const float weight = (float(radius) * 2.0f + 1.0f) * (float(radius) * 2.0f + 1.0f);

    for (int dx = -radius; dx <= radius; dx++)
    {
        for (int dy = -radius; dy <= radius; dy++)
        {
            int2 coord = px + int2(dx, dy);
            float3 color = RawRTTex[coord].rgb;

            m1 += color;
            m2 += color * color;
        }
    }

    mean = m1 / weight;
    float3 variance = (m2 / weight) - (mean * mean);

    stdDev = sqrt(max(variance, 0.0f));
}

#define PLANE_DISTANCE 5.0f
bool PlaneDistanceOcclusion(float3 pos, float3 prevPos, float3 currentNormal)
{
    float3 toCurrent = pos - prevPos;
    float dist = abs(dot(toCurrent, currentNormal));
    return dist > PLANE_DISTANCE;
}

bool NormalDisocclusion(float3 normal, float3 prevNormal)
{
    const float NORMAL_DISTANCE_THRESHOLD = 0.1f;
    float result = pow(abs(dot(normal, prevNormal)), 2.0f);
    if(result > NORMAL_DISTANCE_THRESHOLD)
        return false;

    return true;
}

bool MeshIDDisocclusion(uint2 px, uint2 pxLastFrame, uint2 dims)
{
    RWTexture2D<uint> MeshIDTexture = ResourceDescriptorHeap[Params.MeshIDTex];
    RWTexture2D<uint> PrevMeshIDTexture = ResourceDescriptorHeap[Params.PrevMeshIDTex];
    
    const uint meshID = MeshIDTexture[px];
    const uint meshIDLastFrame = MeshIDTexture[pxLastFrame];

    return meshID != meshIDLastFrame;
}

bool IsReprojectCoordValid(uint2 px, uint2 pxLastFrame, float3 normal, float3 prevNormal, uint2 dims)
{
    bool result = pxLastFrame.x >= 0 && pxLastFrame.y >= 0 && pxLastFrame.x < dims.x && pxLastFrame.y < dims.y;
    if(!result)
        return false;

    float3 curPos = WorldPosFromDepth(DepthTexture[px], px / float2(dims)).xyz;
    float3 prevPos = WorldPosFromDepth(DepthTexture[pxLastFrame], pxLastFrame / float2(dims)).xyz;

    if(MeshIDDisocclusion(px, pxLastFrame, dims))
        return false;

    if(PlaneDistanceOcclusion(curPos, prevPos, normal))
        return false;

    if(NormalDisocclusion(normal, prevNormal))
        return false;
    
    return true;
}

#define ENABLE_ACCUMULATION 0
#define TEMPORAL_FILTER 1

void TemporalFilter(uint2 px, uint2 dims)
{
    RWTexture2D<float3> AccumTex = ResourceDescriptorHeap[Params.AccumulationTexIdx];
    RWTexture2D<float3> PrevAccumTex = ResourceDescriptorHeap[Params.PrevFrameTexIdx];
    RWTexture2D<float>  HistoryLengthTex = ResourceDescriptorHeap[Params.HistoryTexIdx];
    RWTexture2D<float4> PrevNormalsTex = ResourceDescriptorHeap[Params.PrevNormalTexIdx];

    float2 uv               = px / float2(dims);
    float2 motionVec        = GetGBufferMotionVector(px, Frame);
    uint2  pxLastFrame      = uint2(px + motionVec * float2(dims) + float2(0.5f, 0.5f));
    float3 curSample        = RawRTTex[px];
    float3 currentNormals   = GetGBuffersNormal(px, Frame);
    float3 prevNormals      = UnpackNormal(PrevNormalsTex[pxLastFrame].xyz);
        
    const float2 pxLastFrameFloor = floor(px.xy) + motionVec.xy * dims;

    float3 output = curSample;

    const uint2 offsets[4] = { uint2(0, 0), uint2(1, 0), uint2(0, 1), uint2(1, 1) };
    bool v[4];
    bool valid = false;

    for(int idx = 0; idx < 4; ++idx)
    {
        uint2 offset = offsets[idx];
        uint2 pxOffset = px + offset;
        uint2 pxLastFrameOffset = pxLastFrameFloor + offset;
        float3 normalOffset = GetGBuffersNormal(pxOffset, Frame);
        float3 prevNormalOffset = UnpackNormal(PrevNormalsTex[pxLastFrameOffset].xyz);

        v[idx] = IsReprojectCoordValid(pxOffset, pxLastFrameOffset, normalOffset, prevNormalOffset, dims);
        valid = valid || v[idx];
    }

    float3 accumColor = 0.f;

    if(valid)
    {
        float sumw = 0;
        float x    = frac(pxLastFrameFloor.x);
        float y    = frac(pxLastFrameFloor.y);

        // bilinear weights
        float w[4] = { (1 - x) * (1 - y),
                       x * (1 - y),
                       (1 - x) * y,
                       x * y };
        
        for(int idx = 0; idx < 4; ++idx)
        {
            if(v[idx])
            {
                uint2 coord = pxLastFrameFloor + offsets[idx];
                accumColor += w[idx] * PrevAccumTex[coord].xyz;
                sumw += w[idx];
            }
        }

        valid = sumw > 0.01f;
        accumColor = valid ? accumColor / sumw : 0.f;
    }

    if(!valid)
    {
        float count = 0.f;
        const int radius = 1;
        for (int yy = -radius; yy <= radius; yy++)
        {
            for (int xx = -radius; xx <= radius; xx++)
            {
                int2 offset = int2(xx, yy);
                uint2 pxOffset = pxLastFrame + offset;

                float3 prevNormalOffset = UnpackNormal(PrevNormalsTex[pxOffset].xyz);
                if(IsReprojectCoordValid(px, pxOffset, currentNormals, prevNormalOffset, dims))
                {
                    accumColor += PrevAccumTex[pxOffset].xyz;
                    count += 1.f;
                }
            }
        }

        if(count > 0)
        {
            accumColor /= count;
            valid = true;
        }
    }

    float historyLength = 0.0f;
    float accumAlpha = (Params.FrameIndex == 0) ? 1.0f : Params.AccumulationAlpha;

    if(valid)
    {
        float3 stdDev;
        float3 mean;
        NeighbourhoodStdDev(px, mean, stdDev);
        float3 color = RawRTTex[px].rgb;
        // Clamp to 2 standard deviations
        accumColor = ClipAABB(mean - 2.0f * stdDev, mean + 2.0f * stdDev, accumColor);
    }

    if(valid)
    {
        historyLength = HistoryLengthTex[pxLastFrame];
        accumAlpha = max(Params.AccumulationAlpha, 1.f / historyLength); 
        output = lerp(accumColor, curSample, accumAlpha);
    }

    AccumTex[px] = output;
    HistoryLengthTex[px] = min(32.f, historyLength + 1.f);
}

[numthreads(8, 8, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    RWTexture2D<float3> AccumTex = ResourceDescriptorHeap[Params.AccumulationTexIdx];
    uint2 px = DTid.xy;
    uint2 dims = uint2(1920, 1080);

    if(Params.FrameIndex == 0)
    {
        AccumTex[px] = RawRTTex[px];
        return;
    }

#if TEMPORAL_FILTER
        TemporalFilter(DTid.xy, dims);
        return;
#else

#if !ENABLE_ACCUMULATION
    AccumTex[px] = RawRTTex[px];
    return;
#endif
    
    RWTexture2D<float3> PrevAccumTex = ResourceDescriptorHeap[Params.PrevFrameTexIdx];
    RWTexture2D<float4> PrevNormalsTex = ResourceDescriptorHeap[Params.PrevNormalTexIdx];
    RWTexture2D<uint>   HistoryLengthTex = ResourceDescriptorHeap[Params.HistoryTexIdx];

    float accumAlpha = (Params.FrameIndex == 0) ? 1.0f : Params.AccumulationAlpha;
    float3 currentNormals = GetGBuffersNormal(px, Frame);
    float2 motionVec = GetGBufferMotionVector(px, Frame) * float2(dims);
    float2 pxLastFrame = float2(px) - motionVec;
    float3 accumLastFrame = PrevAccumTex[pxLastFrame].xyz;
    float3 raw = RawRTTex[px].xyz;

    const float PrevFrameDistThreshold = 1.f;
    float2 dif = abs(float2(px) - pxLastFrame);

    // Early exit if the pixel is too far from the previous frame
    if(dif.x > PrevFrameDistThreshold || dif.y > PrevFrameDistThreshold)
    {
        AccumTex[px] = raw;
        return;
    }

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

    float3 prevNormals = UnpackNormal(PrevNormalsTex[pxLastFrame].xyz);
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