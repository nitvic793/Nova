#include "Common.hlsli"
#include "GBufferCommon.hlsli"

ConstantBuffer<BlurParams>  Params          : register(b0);
ConstantBuffer<FrameData>   Frame           : register(b1);
Texture2D<float>            DepthTexture    : register(t0);
RWTexture2D<float3>         Output          : register(u0);

float GetDepth(int2 pos)
{
    float depth = GetGBufferDepth(pos, Frame);
    if (depth >= 1.0)
    {
        depth = 100000.0f;
    }
    else
    {
        float2 z = mul(float4(0.0f, 0.0f, depth, 1.0f), Frame.ProjectionInverse).zw;
        depth = z.x / z.y;
    }
    return depth;
}

#define ENABLE_BLUR 1

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 dimensions;
    Output.GetDimensions(dimensions.x, dimensions.y);
    Texture2D<float3> Input = ResourceDescriptorHeap[Params.InputTexIdx];
    
#if ENABLE_BLUR
    const int32_t BlurRadius = Params.BlurRadius;
#else
    const int32_t BlurRadius = 0;
#endif

    uint2 px = DTid.xy;
    float centerDepth = GetDepth(px);
    float3 centerValue = Input[px].xyz;

    float3 value = 0.xxx;

    if (true)
    {
        float weight = 0.0f;
        for (int iy = -BlurRadius; iy <= BlurRadius; ++iy)
        {
            for (int ix = -BlurRadius; ix <= BlurRadius; ++ix)
            {
                int2 samplePx = px + int2(ix, iy);
                if (samplePx.x >= 0 && samplePx.y >= 0 && samplePx.x < dimensions.x && samplePx.y < dimensions.y)
                {
                    float depth = GetDepth(samplePx);
                    float threshold = Params.BlurDepthThreshold;
                    if (abs(depth - centerDepth) <= threshold)
                    {
                        value += Input[samplePx].xyz;
                        weight += 1.0f;
                    }
                }
            }
        }
        value /= weight;
    }
    else
    {
        value = centerValue;
    }
    
    Output[px] = value;
}