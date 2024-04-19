#include "Common.hlsli"
#include "Lighting.hlsli"
#include "GBufferCommon.hlsli"

ConstantBuffer<ObjectData>   Object : register(b0);
ConstantBuffer<FrameData>    Frame    : register(b1);

SamplerState    BasicSampler		: register(s0);
SamplerState    LinearWrapSampler	: register(s2);

struct PixelOutput
{
	float4 AlbedoRoughness		: SV_TARGET0;
    float4 NormalMetalness	    : SV_TARGET1;
	float4 WorldPos		        : SV_TARGET2;
    float4 MotionVector         : SV_TARGET3;
};

float2 GetVelocity(PixelInput input)
{
    float2 prevScreenPos = input.PrevPosition.xy / input.PrevPosition.w;
    float2 currentScreenPos = input.CurPosition.xy/ input.CurPosition.w;
    prevScreenPos = prevScreenPos * 0.5f + 0.5f;
    currentScreenPos = currentScreenPos * 0.5f + 0.5f;
    return prevScreenPos - currentScreenPos;
}

PixelOutput main(PixelInput input)
{
    ConstantBuffer<MaterialData> Material = ResourceDescriptorHeap[Object.MaterialIndex];
    Texture2D<float4> albedoTex = ResourceDescriptorHeap[Material.AlbedoOffset]; 
    Texture2D<float3> normalTex = ResourceDescriptorHeap[Material.NormalOffset];
    Texture2D<float> roughnessTex = ResourceDescriptorHeap[Material.RoughnessOffset];
    Texture2D<float> metallicTex = ResourceDescriptorHeap[Material.MetalnessOffset];

    float3 albedo = albedoTex.Sample(LinearWrapSampler, input.UV).xyz; 
    float3 normalSample = normalTex.Sample(LinearWrapSampler, input.UV).xyz;
    float3 normal = CalculateNormalFromSample(normalSample, input.UV, normalize(input.Normal), input.Tangent);
    float roughness = roughnessTex.Sample(LinearWrapSampler, input.UV);
    float metallic = metallicTex.Sample(LinearWrapSampler, input.UV);

    PixelOutput output;
    output.AlbedoRoughness = float4(albedo, roughness);
    output.NormalMetalness = float4(PackNormal(normal), metallic);
    output.WorldPos = float4(input.WorldPos.xyz, 1.f); // fourth component is unused
    output.MotionVector = float4(GetVelocity(input), 0.xx);

    return output; 
}