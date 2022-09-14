#include "Common.hlsli"

ConstantBuffer<ObjectData> Object: register(b0);
ConstantBuffer<FrameData> Frame: register(b1);

PixelInput main(VertexInput input)
{
    PixelInput output;
    float4 worldPos = mul(float4(input.Position, 1.0f), Object.World);
    float4x4 wvp = mul(Object.World, mul(Frame.View, Frame.Projection));

    output.Position = mul(float4(input.Position, 1.f), wvp);
	output.Normal = normalize(mul(input.Normal, (float3x3)Object.World));
	output.UV = input.UV;
	output.Tangent = normalize(mul(input.Tangent, (float3x3)Object.World));;
    output.WorldPos = worldPos.xyz;
    return output;
}