#include "Common.hlsli"
#include "Lighting.hlsli"

ConstantBuffer<MaterialData>    Material: register(b0);
ConstantBuffer<FrameData>       Frame: register(b1);

Texture2D<float4> Albedo : register(t0);

SamplerState    BasicSampler		: register(s0);
SamplerState    LinearWrapSampler	: register(s2);

float4 main(PixelInput input) : SV_TARGET
{
    Texture2D<float4> myTexture = ResourceDescriptorHeap[Material.AlbedoOffset]; 
    float3 albedo = myTexture.Sample(LinearWrapSampler, input.UV).xyz; 

    float3 normal = normalize(input.Normal);
    float3 light = CalculateDirectionalLight(normal, Frame.DirLights[0]);
    return float4(albedo * light, 1); 
}