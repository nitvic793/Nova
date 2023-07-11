#include "Common.hlsli"

ConstantBuffer<ObjectData>  Object      : register(b0);
ConstantBuffer<FrameData>   Frame       : register(b1);
ConstantBuffer<PerArmature> Armature    : register(b2);

float4x4 SkinTransform(float4 weights, uint4 boneIndices, in float4x4 bones[MAX_BONES])
{
	// Calculate the skin transform from up to four bones and weights
    float4x4 skinTransform =
			bones[boneIndices.x] * weights.x +
			bones[boneIndices.y] * weights.y +
			bones[boneIndices.z] * weights.z +
			bones[boneIndices.w] * weights.w;
    return skinTransform;
}

void SkinVertex(inout float4 position, inout float3 normal, inout float3 tangent, float4x4 skinTransform)
{
    position = mul(position, skinTransform);
    normal = mul(normalize(normal), (float3x3) skinTransform);
    tangent = mul(tangent, (float3x3) skinTransform);
}

PixelInput main(VertexAnimatedInput input)
{
    PixelInput output;
    float4x4 skinTransform = SkinTransform(input.SkinWeights, input.SkinIndices, Armature.Bones);
    
    float4 position = float4(input.Position, 1.f);
    if(input.SkinWeights.x !=0)
    {
        SkinVertex(position, input.Normal, input.Tangent, skinTransform);
    }

    float4 worldPos = mul(position, Object.World);
    float4x4 wvp = mul(Object.World, mul(Frame.View, Frame.Projection));

    output.Position = mul(position, wvp);
	output.Normal = normalize(mul(input.Normal, (float3x3)Object.World));
	output.UV = input.UV;
	output.Tangent = normalize(mul(input.Tangent, (float3x3)Object.World));;
    output.WorldPos = worldPos.xyz;
    return output;
}