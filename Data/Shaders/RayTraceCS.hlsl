#include "Common.hlsli"

ConstantBuffer<TraceParams> Params  : register(b0);
RWTexture2D<float3> OutputTexture   : register(u0);

float3 Trace()
{
    return float3(1, 0, 0);
}

[numthreads(1, 1, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    OutputTexture[DTid.xy] = Trace();
}