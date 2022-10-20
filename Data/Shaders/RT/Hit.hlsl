#include "Common.hlsli"
#include "../Interop/ShaderInteropTypes.h"

// #DXR Extra: Per-Instance Data
ConstantBuffer<ObjectData> Object : register(b0);
StructuredBuffer<Vertex> BTriVertex : register(t0);

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
    float3 barycentrics =
        float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    ConstantBuffer<MaterialData> Material = ResourceDescriptorHeap[Object.MaterialIndex];
    Texture2D<float4> albedoTex = ResourceDescriptorHeap[Material.AlbedoOffset]; 
    float3 albedo = float3(0.7, 0.7, 0.3);//albedoTex.Sample(LinearWrapSampler, input.UV).xyz; 

    // #DXR Extra: Per-Instance Data
    float3 hitColor = albedo.x * barycentrics.x + albedo.y * barycentrics.y + albedo.z * barycentrics.z;
    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}

// #DXR Extra: Per-Instance Data
[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 hitColor = float3(0.7, 0.7, 0.3);

    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
