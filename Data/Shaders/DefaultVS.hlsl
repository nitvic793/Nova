#include "Common.hlsli"

ConstantBuffer<ObjectData> Object: register(b0);
ConstantBuffer<FrameData> Frame: register(b1);

PixelInput main(VertexInput input)
{
    PixelInput output;
    float4 worldPos = mul(float4(input.Position, 1.0f), Object.World);
    float4x4 wvp = mul(Object.World, mul(Frame.View, Frame.Projection));
    float4x4 prevWvp = mul(Object.PrevWorld, mul(Frame.PrevView, Frame.PrevProjection));
    float4 pos = mul(float4(input.Position, 1.f), wvp);
    output.Position = pos;
    output.Normal = normalize(mul(input.Normal, (float3x3) Object.World));
	output.UV = input.UV;
	output.Tangent = normalize(mul(input.Tangent, (float3x3)Object.World));;
    output.WorldPos = worldPos.xyz;
    output.ScreenPos = output.Position.xy / output.Position.w;
    output.CurPosition = mul(float4(input.Position, 1.f), wvp);
    output.PrevPosition = mul(float4(input.Position, 1.f), prevWvp);
    return output;
}