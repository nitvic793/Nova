#include "Interop/ShaderInteropTypes.h"

struct VertexInput
{
	float3 Position : POSITION;
    float2 UV		: TEXCOORD;
	float3 Normal	: NORMAL;
	float3 Tangent	: TANGENT0;
};

struct VertexAnimatedInput
{
    float3 Position		: POSITION;
    float2 UV			: TEXCOORD;
    float3 Normal		: NORMAL;
    float3 Tangent		: TANGENT0;
    uint4  SkinIndices	: BLENDINDICES;
    float4 SkinWeights	: BLENDWEIGHT;
};

struct PixelInput
{
	float4 Position		: SV_POSITION;
	float2 UV			: TEXCOORD;
	float3 Normal		: NORMAL;
	float3 Tangent		: TANGENT;
	float3 WorldPos		: POSITION0;
};

