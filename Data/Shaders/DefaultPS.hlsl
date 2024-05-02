#include "Common.hlsli"
#include "Lighting.hlsli"

ConstantBuffer<ObjectData>   Object : register(b0);
ConstantBuffer<FrameData>    Frame    : register(b1);

Texture2D<float4> LightAccumBuffer : register(t0);

SamplerState    BasicSampler		: register(s0);
SamplerState    LinearWrapSampler	: register(s2);

float4 main(PixelInput input) : SV_TARGET
{
    ConstantBuffer<MaterialData> Material = ResourceDescriptorHeap[Object.MaterialIndex];
    Texture2D<float4> albedoTex = ResourceDescriptorHeap[Material.AlbedoOffset]; 
    Texture2D<float3> normalTex = ResourceDescriptorHeap[Material.NormalOffset];
    float3 albedo = albedoTex.Sample(LinearWrapSampler, input.UV).xyz; 
    float3 normalSample = normalTex.Sample(LinearWrapSampler, input.UV).xyz;

    float3 normal = CalculateNormalFromSample(normalSample, input.UV, normalize(input.Normal), input.Tangent);
    float3 light = CalculateDirectionalLight(normal, Frame.DirLights[0]);
    float3 accumLight = float3(1, 1, 1); // LightAccumBuffer.Sample(LinearWrapSampler, input.ScreenPos).xyz;
    return float4(albedo * light * accumLight, 1); 
}