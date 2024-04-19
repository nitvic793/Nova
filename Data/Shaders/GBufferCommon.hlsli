#include "Interop/ShaderInteropTypes.h"

float3 PackNormal(float3 normal)
{
    return normal * 0.5f + 0.5f;
}

float3 UnpackNormal(float3 normal)
{
    return normal * 2.0f - 1.0f;
}

/*
    GBuffer layout:
    A: AlbedoRoughness (Albedo.xyz, Roughness) | RGBA8
    B: NormalMetalness (Normal.xyz, Metalness) | RGBA8
    C: WorldPos (WorldPos.xyz, unused) | RGBA16F
*/

float3 GetGBuffersNormal(int2 px, ConstantBuffer<FrameData> frame)
{
    Texture2D<float4> gBufferB = ResourceDescriptorHeap[frame.GBufferBIdx];
    return UnpackNormal(gBufferB.Load(int3(px, 0)).xyz);
}

float3 GetGBuffersWorldPos(int2 px, ConstantBuffer<FrameData> frame)
{
    Texture2D<float4> gBufferC = ResourceDescriptorHeap[frame.GBufferCIdx];
    return gBufferC.Load(int3(px, 0)).xyz;
}

float3 GetGBufferAlbedo(int2 px, ConstantBuffer<FrameData> frame)
{
    Texture2D<float4> gBufferA = ResourceDescriptorHeap[frame.GBufferAIdx];
    return gBufferA.Load(int3(px, 0)).xyz;
}

float GetGBufferRoughness(int2 px, ConstantBuffer<FrameData> frame)
{
    Texture2D<float4> gBufferA = ResourceDescriptorHeap[frame.GBufferAIdx];
    return gBufferA.Load(int3(px, 0)).w;
}

float GetGBufferMetalness(int2 px, ConstantBuffer<FrameData> frame)
{
    Texture2D<float4> gBufferB = ResourceDescriptorHeap[frame.GBufferBIdx];
    return gBufferB.Load(int3(px, 0)).w;
}

float GetGBufferDepth(int2 px, ConstantBuffer<FrameData> frame)
{
    Texture2D<float> gBufferD = ResourceDescriptorHeap[frame.GBufferDepthIdx];
    return gBufferD.Load(int3(px, 0));
}

float2 GetGBufferMotionVector(int2 px, ConstantBuffer<FrameData> frame)
{
    Texture2D<float4> gBufferD = ResourceDescriptorHeap[frame.GBufferDIdx];
    return gBufferD.Load(int3(px, 0)).xy;
}