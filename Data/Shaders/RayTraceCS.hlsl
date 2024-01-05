#include "Common.hlsli"
#include "Lighting.hlsli"

ConstantBuffer<TraceParams> Params          : register(b0);
ConstantBuffer<FrameData>   Frame           : register(b1);
RWTexture2D<float3>         OutputTexture   : register(u0);
StructuredBuffer<Vertex>    SkyTexture      : register(t0);

SamplerState            LinearWrapSampler	: register(s0);

// Define 1/pi
#define INV_PI      0.318309886183790671538
#define INV2PI		0.15915494309189533576888f
#define PI			3.14159265358979323846264f

// 
float3 LinearToSRGB(float3 linearCol)
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = (pow(abs(linearCol), float3(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4)) * 1.055) - 0.055;
	float3 sRGB;
	sRGB.r = linearCol.r <= 0.0031308 ? sRGBLo.r : sRGBHi.r;
	sRGB.g = linearCol.g <= 0.0031308 ? sRGBLo.g : sRGBHi.g;
	sRGB.b = linearCol.b <= 0.0031308 ? sRGBLo.b : sRGBHi.b;
	return sRGB;
}

// Ref: http://cwyman.org/code/dxrTutors/tutors/Tutor10/tutorial10.md.html
// Convert our world space direction to a (u,v) coord in a latitude-longitude spherical map
float2 wsVectorToLatLong(float3 dir)
{
	float3 p = normalize(dir);
	float u = (1.f + atan2(p.x, -p.z) * INV_PI) * 0.5f;
	float v = acos(p.y) * INV_PI;
	return float2(u, v);
}

float3 Trace(Ray ray, uint3 dispatchTId)
{
    float x = Frame.CameraPosition.x;
    float red = dispatchTId.x / (Params.Resolution.x * Params.ScaleFactor);
    float green = dispatchTId.y / (Params.Resolution.y * Params.ScaleFactor);
    return float3(red, green, x);
}

uint2 GetSkyUV( Ray ray, uint2 texDims )
{
    float phi = atan2( ray.Dir.z, ray.Dir.x );
    uint u = (uint)(texDims.x * (phi > 0 ? phi : (phi + 2 * PI)) * INV2PI - 0.5f);
    uint v = (uint)(texDims.y * acos( ray.Dir.y ) * INV_PI - 0.5f);
    return uint2(u, v);
}

inline void GenerateCameraRay(uint2 idx, out Ray ray)
{
    float2 dimensions = (Params.Resolution.xy * Params.ScaleFactor);
    // float2 xy = idx + 0.5f;
    // float2 screenPos = xy / (Params.Resolution.xy * Params.ScaleFactor) * 2.0 - 1.0;
    // screenPos.y = -screenPos.y;

    float2 rayPixelPos = idx;
    float2 ncdXY = (rayPixelPos / dimensions.xy * 0.5f) - 1.0f; // Transfrom [0,1] to [-1, 1]
    ncdXY.y *= -1.0f;

    float4 rayStart = mul(float4(ncdXY, 0.0f, 1.0f), Frame.ViewProjectionInverse); // Near clip plane
    float4 rayEnd = mul(float4(ncdXY, 1.0f, 1.0f), Frame.ViewProjectionInverse); // Far clip plane
    rayStart.xyz /= rayStart.w;
    rayEnd.xyz /= rayEnd.w;
    float3 rayDir = normalize(rayEnd.xyz - rayStart.xyz);

    //float4 world = mul(float4(screenPos, 0, 1), Frame.ViewProjectionInverse);

    ray.Orig = rayStart.xyz;
    ray.Dir = rayDir;
    ray.Hit.T = 1e30f; 
}

void GetRayDesc(uint2 px, out RayDesc ray)
{
    uint w, h;
    OutputTexture.GetDimensions(w, h);
    float2 dims = float2(w, h);
    float2 screenPos = (float2(px) + 0.5f)/ dims * 2.0 - 1.0;
    screenPos.y = -screenPos.y;

    float4 world = mul(float4(screenPos, Frame.NearZ, 1), Frame.ViewProjectionInverse);
    world.xyz /= world.w;

    ray.Origin = Frame.CameraPosition;
    ray.TMin = 0.1f;
    ray.TMax = 10000.f;
    ray.Direction = normalize(world.xyz - ray.Origin);
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 TriangleNormal(float3 vertexAttribute[3], float2 bary)
{
    return normalize(vertexAttribute[0] +
        bary.x * (vertexAttribute[1] - vertexAttribute[0]) +
        bary.y * (vertexAttribute[2] - vertexAttribute[0]));
}

float2 GetUV(float2 barycentrics, float2 uv0, float2 uv1, float2 uv2)
{
    float2 b = barycentrics;
    float2 uv = b.x * uv1 + b.y * uv2 + (1-(b.x + b.y))*uv0;
    return uv;
}

float4 DoInlineRayTracing(RayDesc ray)
{
    const float4 HIT_COLOR = float4(1,0,0,1);
    const float4 MISS_COLOR = float4(0,0,0,1);
    float4 result = 0.xxxx;


    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[Params.RTSceneIdx];
    StructuredBuffer<Vertex> Mesh = ResourceDescriptorHeap[Params.VertexBufferIdx];
    StructuredBuffer<uint> IndexBuffer = ResourceDescriptorHeap[Params.IndexBufferIdx];

    RayQuery<RAY_FLAG_CULL_NON_OPAQUE |
	RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
	RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;

	rayQuery.TraceRayInline(
		Scene,
		0,
		255,
		ray
	);

	rayQuery.Proceed();

	if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
        ConstantBuffer<ObjectData> object = ResourceDescriptorHeap[Params.ObjectDataIdx];
        ConstantBuffer<MaterialData> Material = ResourceDescriptorHeap[object.MaterialIndex];
        Texture2D<float4> albedoTex = ResourceDescriptorHeap[Material.AlbedoOffset]; 

        float2 baryUv = rayQuery.CandidateTriangleBarycentrics();
        uint index0 = IndexBuffer[rayQuery.CandidatePrimitiveIndex() * 3];
        uint index1 = IndexBuffer[rayQuery.CandidatePrimitiveIndex() * 3 + 1];
        uint index2 = IndexBuffer[rayQuery.CandidatePrimitiveIndex() * 3 + 2];
        float2 uv0 = Mesh[index0].mUV;
        float2 uv1 = Mesh[index1].mUV;
        float2 uv2 = Mesh[index2].mUV;

        float3 vertexNormals[3] = { 
            Mesh[index0].mNormal , 
            Mesh[index1].mNormal , 
            Mesh[index2].mNormal  
        };

        float3 triNormal = TriangleNormal(vertexNormals, baryUv);


        float2 uv = GetUV(baryUv, uv0, uv1, uv2);

        const float minT = 0.1f;
        const float tAtWhich1x1 = 200;  // Depends on the FOV of the "camera". A surface normal could also help here.
        const float maxDim = 1024;//Get Dim from tex and (float)max(width, height);
        const float grad = minT / (tAtWhich1x1 * maxDim);
        // TODO: calculate gradients by method given here: 
        // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12Raytracing/src/D3D12RaytracingMiniEngineSample/DiffuseHitShaderLib.hlsl#L233
        float3 albedo = albedoTex.SampleGrad(LinearWrapSampler, uv, float2(grad, 0), float2(0, grad)).xyz; 

        float3 light = CalculateDirectionalLight(triNormal, Frame.DirLights[0]);
        result = float4(LinearToSRGB(light * albedo), 1.f);
	}
	else
		result = MISS_COLOR;

    return result;
} 

[numthreads(8, 8, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray;
    GenerateCameraRay(DTid.xy, ray);

    // float2 texDims;
    // SkyTexture.GetDimensions( texDims.x, texDims.y );

    // float2 uv = GetSkyUV(ray, texDims);

    // if(Params.StructBufferIdx > 0)
    // { 
    //     StructuredBuffer<uint> testBuffer = ResourceDescriptorHeap[Params.StructBufferIdx];
    //     uv = float(testBuffer[0]) * uv;
    // }

    RayDesc rayDesc;
    GetRayDesc(DTid.xy, rayDesc);
    OutputTexture[DTid.xy] = DoInlineRayTracing(rayDesc).xyz;//SkyTexture[uv].xyz;//Trace(ray, DTid);
}