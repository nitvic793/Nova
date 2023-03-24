#include "Common.hlsli"

ConstantBuffer<TraceParams> Params          : register(b0);
ConstantBuffer<FrameData>   Frame           : register(b1);
RWTexture2D<float3>         OutputTexture   : register(u0);

float3 Trace(uint3 dispatchTId)
{
    float x = Frame.CameraPosition.x;
    float red = dispatchTId.x / (Params.Resolution.x * Params.ScaleFactor);
    float green = dispatchTId.y / (Params.Resolution.y * Params.ScaleFactor);
    return float3(red, green, x);
}

[numthreads(8, 8, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    OutputTexture[DTid.xy] = Trace(DTid);
}