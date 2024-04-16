#include "Common.hlsli"
#include "Lighting.hlsli"

// Inputs expected when Raytracing
ConstantBuffer<TraceParams> Params              : register(b0);
ConstantBuffer<FrameData>   Frame               : register(b1);
SamplerState                LinearWrapSampler   : register(s0);

StructuredBuffer<MeshInstanceData> MeshInstances : register(t0);

// Define 1/pi
#define INV_PI      0.318309886183790671538
#define INV2PI		0.15915494309189533576888f
#define PI			3.14159265358979323846264f
#define MAX_DIST    10000.f
#define MIN_DIST    0.1f

typedef RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> DefaultRayQueryT;

struct ShadeContext
{
    float3 Color;
    float3 Normal;
    float3 WorldPos;
    float2 BaryUV;
    float2 UV;
    float  Roughness;
    float  Metallic;
};

struct HitContext
{
    float3 Color;
    float3 WorldNormal;
    float3 WorldPos;
    float  ShadowVisibility;
};

float3 getPerpendicularVector(float3 u)
{
	float3 a = abs(u);
	uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
	uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, float3(xm, ym, zm));
}

// From: https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/f1946147ea50987efd4e897d8bb996e2f8bc99df/CommonPasses/Data/CommonPasses/thinLensUtils.hlsli#L21
uint InitRand(uint val0, uint val1, uint backoff = 16)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float NextRand(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

// From: https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/f1946147ea50987efd4e897d8bb996e2f8bc99df/DXR-Sphereflake/Data/Sphereflake/randomUtils.hlsli#L43
// Get a cosine-weighted random vector centered around a specified normal direction.
float3 GetCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
	// Get 2 random numbers to select our sample with
	float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	float3 bitangent = getPerpendicularVector(hitNorm);
	float3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(randVal.x);
	float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(max(0.0, 1.0f - randVal.x));
}

// From: https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/f1946147ea50987efd4e897d8bb996e2f8bc99df/CommonPasses/Data/CommonPasses/simpleDiffuseGIUtils.hlsli#L131
// Get a uniform weighted random vector centered around a specified normal direction.
float3 GetUniformHemisphereSample(inout uint randSeed, float3 hitNorm)
{
	// Get 2 random numbers to select our sample with
	float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	float3 bitangent = getPerpendicularVector(hitNorm);
	float3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(max(0.0f,1.0f - randVal.x*randVal.x));
	float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * randVal.x;
}

// Credits: Alan Wolfe
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

// Credits: Alan Wolfe
float3 SRGBToLinear(in float3 sRGBCol)
{
	float3 linearRGBLo = sRGBCol / 12.92;
	float3 linearRGBHi = pow((sRGBCol + 0.055) / 1.055, float3(2.4, 2.4, 2.4));
	float3 linearRGB;
	linearRGB.r = sRGBCol.r <= 0.04045 ? linearRGBLo.r : linearRGBHi.r;
	linearRGB.g = sRGBCol.g <= 0.04045 ? linearRGBLo.g : linearRGBHi.g;
	linearRGB.b = sRGBCol.b <= 0.04045 ? linearRGBLo.b : linearRGBHi.b;
	return linearRGB;
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

// Credits: Microsoft - https://github.com/microsoft/DirectX-Graphics-Samples/blob/b5f92e2251ee83db4d4c795b3cba5d470c52eaf8/Samples/Desktop/D3D12Raytracing/src/D3D12RaytracingSimpleLighting/Raytracing.hlsl#L71
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

ShadeContext GetShadeContext(uint instanceId, DefaultRayQueryT rayQuery)
{
    MeshInstanceData instanceData       = MeshInstances[instanceId];
    StructuredBuffer<Vertex> Mesh       = ResourceDescriptorHeap[instanceData.VertexBufferIdx];
    StructuredBuffer<uint> IndexBuffer  = ResourceDescriptorHeap[instanceData.IndexBufferIdx];

    ConstantBuffer<ObjectData> object       = ResourceDescriptorHeap[instanceData.ObjectDataIdx];
    ConstantBuffer<MaterialData> Material   = ResourceDescriptorHeap[object.MaterialIndex];
    Texture2D<float4> albedoTex             = ResourceDescriptorHeap[Material.AlbedoOffset]; 
    Texture2D<float3> normalTex             = ResourceDescriptorHeap[Material.NormalOffset];
    Texture2D<float> roughnessTex           = ResourceDescriptorHeap[Material.RoughnessOffset];
    Texture2D<float> metallicTex            = ResourceDescriptorHeap[Material.MetalnessOffset];

    uint index0 = IndexBuffer[rayQuery.CandidatePrimitiveIndex() * 3];
    uint index1 = IndexBuffer[rayQuery.CandidatePrimitiveIndex() * 3 + 1];
    uint index2 = IndexBuffer[rayQuery.CandidatePrimitiveIndex() * 3 + 2];

    float2 uv0 = Mesh[index0].mUV;
    float2 uv1 = Mesh[index1].mUV;
    float2 uv2 = Mesh[index2].mUV;

    float3 vertexNormals[3] = 
    { 
        Mesh[index0].mNormal , 
        Mesh[index1].mNormal , 
        Mesh[index2].mNormal  
    };
    
    float3 vertexTangents[3] = 
    { 
        Mesh[index0].mTangent , 
        Mesh[index1].mTangent , 
        Mesh[index2].mTangent  
    };

    float2 baryUv = rayQuery.CandidateTriangleBarycentrics();
    float3 triNormal = TriangleNormal(vertexNormals, baryUv);
    float3 triTangent = TriangleNormal(vertexTangents, baryUv);
    float2 uv = GetUV(baryUv, uv0, uv1, uv2);

    const float minT = 0.1f;
    const float tAtWhich1x1 = 200;  // Depends on the FOV of the "camera". A surface normal could also help here.
    const float maxDim = 1024;//Get Dim from tex and (float)max(width, height);
    const float grad = minT / (tAtWhich1x1 * maxDim);

    // TODO: calculate gradients by method given here: 
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12Raytracing/src/D3D12RaytracingMiniEngineSample/DiffuseHitShaderLib.hlsl#L233
    float3 worldPos = rayQuery.WorldRayOrigin() + rayQuery.WorldRayDirection() * rayQuery.CommittedRayT();
    float2 ddx = float2(grad, 0);
    float2 ddy = float2(0, grad);
    float3 albedo = albedoTex.SampleGrad(LinearWrapSampler, uv, ddx, ddy).xyz; 
    float3 normal = normalTex.SampleGrad(LinearWrapSampler, uv, ddx, ddy).xyz;
    normal = CalculateNormalFromSample(normal, uv, triNormal, triTangent);

    ShadeContext ctx;

    ctx.Normal = normal;
    ctx.WorldPos = worldPos;
    ctx.UV = uv;
    ctx.BaryUV = baryUv;
    ctx.Color = albedo;

    return ctx;
}

DefaultRayQueryT ShootRay(RayDesc ray)
{
    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[Params.RTSceneIdx];
    DefaultRayQueryT rayQuery;

    rayQuery.TraceRayInline(
		Scene,
		0,
		255,
		ray
	);

    rayQuery.Proceed();
    return rayQuery;
}

DefaultRayQueryT ShootRay(float3 origin, float3 direction, float minT, float maxT)
{
    RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = minT;
	ray.TMax = maxT;

    return ShootRay(ray);
}

// From: https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/f1946147ea50987efd4e897d8bb996e2f8bc99df/13-SimpleToneMapping/Data/Tutorial13/standardShadowRay.hlsli#L31
float ShadowRayVisibility(float3 origin, float3 direction, float minT, float maxT)
{
	// Setup our shadow ray
	DefaultRayQueryT rayQuery = ShootRay(origin, direction, minT, maxT);

    float hitDist = MAX_DIST;
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        hitDist = rayQuery.CommittedRayT();
	// Check if anyone was closer than our maxT distance (in which case we're occluded)
	return (hitDist > maxT) ? 1.0f : 0.0f;
}

float3 OnHit(uint instanceId, DefaultRayQueryT rayQuery, out HitContext hitContext)
{
    ShadeContext ctx = GetShadeContext(instanceId, rayQuery);

    DirectionalLight dirLight = Frame.DirLights[0];
    float3 toLight = normalize(-dirLight.Direction);
    float shadowVisibility = Params.EnableShadows ? ShadowRayVisibility(ctx.WorldPos, toLight, MIN_DIST, MAX_DIST - 100.f) : 1.0f;
    float3 light = CalculateDirectionalLight(ctx.Normal, dirLight) * shadowVisibility;

    float3 color = light * ctx.Color;

    hitContext.WorldPos = ctx.WorldPos;
    hitContext.WorldNormal = ctx.Normal;
    hitContext.Color = ctx.Color;
    hitContext.ShadowVisibility = shadowVisibility;
    return color;
}

float3 OnMiss(float3 rayDir, DefaultRayQueryT rayQuery)
{
    TextureCube skybox = ResourceDescriptorHeap[Params.SkyBoxHandle];
    float3 color = skybox.Sample(LinearWrapSampler, rayDir).xyz;
    return color;
}

float3 ShootIndirectRay(float3 worldPos, float3 dir, float minT, uint seed)
{
    DefaultRayQueryT rayQuery = ShootRay(worldPos, dir, minT, MAX_DIST);
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        HitContext ctx;
        uint instanceId = rayQuery.CommittedInstanceID(); // Used to index into array of structs to get data needed to calculate light
        return OnHit(instanceId, rayQuery, ctx);
    }
        
    return OnMiss(dir, rayQuery);
}

// Reference: https://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
// GGX Microfacet BRDF

float ggxNormalDistribution( float NdotH, float roughness )
{
	float a2 = roughness * roughness;
	float d = ((NdotH * a2 - NdotH) * NdotH + 1);
	return a2 / (d * d * PI);
}

float ggxSchlickMaskingTerm(float NdotL, float NdotV, float roughness)
{
	// Karis notes they use alpha / 2 (or roughness^2 / 2)
	float k = roughness * roughness / 2;

	// Compute G(v) and G(l).  These equations directly from Schlick 1994
	float g_v = NdotV / (NdotV*(1 - k) + k);
	float g_l = NdotL / (NdotL*(1 - k) + k);
	return g_v * g_l;
}

float3 schlickFresnel(float3 f0, float lDotH)
{
	return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - lDotH, 5.0f);
}

float3 getGGXMicrofacet(inout uint randSeed, float roughness, float3 hitNorm)
{
	// Get our uniform random numbers
	float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

	// Get an orthonormal basis from the normal
	float3 B = getPerpendicularVector(hitNorm);
	float3 T = cross(B, hitNorm);

	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cosThetaH = sqrt(max(0.0f, (1.0-randVal.x)/((a2-1.0)*randVal.x+1) ));
	float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
	float phiH = randVal.y * PI * 2.0f;

	// Get our GGX NDF sample (i.e., the half vector)
	return T * (sinThetaH * cos(phiH)) +
           B * (sinThetaH * sin(phiH)) +
           hitNorm * cosThetaH;
}

float3 ggxDirect(inout uint rndSeed, float3 hit, float3 N, float3 V, float3 dif, float3 spec, float rough, float3 toLight, float intensity)
{
	// Query the scene to find info about the randomly selected light
	float distToLight;
	float3 lightIntensity = intensity.xxx;
	float3 L = toLight;

	// Compute our lambertion term (N dot L)
	float NdotL = saturate(dot(N, L));

	// Shoot our shadow ray to our randomly selected light
	float shadowMult =  ShadowRayVisibility(hit, L, MIN_DIST, distToLight);

	// Compute half vectors and additional dot products for GGX
	float3 H = normalize(V + L);
	float NdotH = saturate(dot(N, H));
	float LdotH = saturate(dot(L, H));
	float NdotV = saturate(dot(N, V));

	// Evaluate terms for our GGX BRDF model
	float  D = ggxNormalDistribution(NdotH, rough);
	float  G = ggxSchlickMaskingTerm(NdotL, NdotV, rough);
	float3 F = schlickFresnel(spec, LdotH);

	// Evaluate the Cook-Torrance Microfacet BRDF model
	//     Cancel out NdotL here & the next eq. to avoid catastrophic numerical precision issues.
	float3 ggxTerm = D*G*F / (4 * NdotV /* * NdotL */);

	// Compute our final color (combining diffuse lobe plus specular GGX lobe)
	return shadowMult * lightIntensity * ( /* NdotL * */ ggxTerm + NdotL * dif / PI);
}

float luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float probabilityToSampleDiffuse(float3 difColor, float3 specColor)
{
	float lumDiffuse = max(0.01f, luminance(difColor.rgb));
	float lumSpecular = max(0.01f, luminance(specColor.rgb));
	return lumDiffuse / (lumDiffuse + lumSpecular);
}

float3 ggxIndirect(inout uint rndSeed, float3 hit, float3 N, float3 noNormalN, float3 V, float3 dif, float3 spec, float rough, uint rayDepth)
{
	// We have to decide whether we sample our diffuse or specular/ggx lobe.
	float probDiffuse = probabilityToSampleDiffuse(dif, spec);
	float chooseDiffuse = (NextRand(rndSeed) < probDiffuse);

	// We'll need NdotV for both diffuse and specular...
	float NdotV = saturate(dot(N, V));

	// If we randomly selected to sample our diffuse lobe...
	if (chooseDiffuse)
	{
		// Shoot a randomly selected cosine-sampled diffuse ray.
		float3 L = GetCosHemisphereSample(rndSeed, N);
		float3 bounceColor = ShootIndirectRay(hit, L, MIN_DIST, rndSeed);

		// Check to make sure our randomly selected, normal mapped diffuse ray didn't go below the surface.
		if (dot(noNormalN, L) <= 0.0f) bounceColor = float3(0, 0, 0);

		// Accumulate the color: (NdotL * incomingLight * dif / pi) 
		// Probability of sampling:  (NdotL / pi) * probDiffuse
		return bounceColor * dif / probDiffuse;
	}
	// Otherwise we randomly selected to sample our GGX lobe
	else
	{
		// Randomly sample the NDF to get a microfacet in our BRDF to reflect off of
		float3 H = getGGXMicrofacet(rndSeed, rough, N);

		// Compute the outgoing direction based on this (perfectly reflective) microfacet
		float3 L = normalize(2.f * dot(V, H) * H - V);

		// Compute our color by tracing a ray in this direction
		float3 bounceColor = ShootIndirectRay(hit, L, MIN_DIST, rndSeed);

		// Check to make sure our randomly selected, normal mapped diffuse ray didn't go below the surface.
		if (dot(noNormalN, L) <= 0.0f) bounceColor = float3(0, 0, 0);

		// Compute some dot products needed for shading
		float  NdotL = saturate(dot(N, L));
		float  NdotH = saturate(dot(N, H));
		float  LdotH = saturate(dot(L, H));

		// Evaluate our BRDF using a microfacet BRDF model
		float  D = ggxNormalDistribution(NdotH, rough);          // The GGX normal distribution
		float  G = ggxSchlickMaskingTerm(NdotL, NdotV, rough);   // Use Schlick's masking term approx
		float3 F = schlickFresnel(spec, LdotH);                  // Use Schlick's approx to Fresnel
		float3 ggxTerm = D * G * F / (4 * NdotL * NdotV);        // The Cook-Torrance microfacet BRDF

		// What's the probability of sampling vector H from getGGXMicrofacet()?
		float  ggxProb = D * NdotH / (4 * LdotH);

		// Accumulate the color:  ggx-BRDF * incomingLight * NdotL / probability-of-sampling
		//    -> Should really simplify the math above.
		return NdotL * bounceColor * ggxTerm / (ggxProb * (1.0f - probDiffuse));
	}
}