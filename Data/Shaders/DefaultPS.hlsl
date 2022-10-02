#include "Common.hlsli"

ConstantBuffer<MaterialData> Material: register(b0);

Texture2D<float4> Albedo : register(t0);

SamplerState    BasicSampler		: register(s0);
SamplerState    LinearWrapSampler	: register(s2);

float4 main(PixelInput input) : SV_TARGET
{
    Texture2D<float4> myTexture = ResourceDescriptorHeap[Material.AlbedoOffset]; 
    float3 albedo = myTexture.Sample(LinearWrapSampler, input.UV).xyz; 
    return float4(albedo, 1); 
}