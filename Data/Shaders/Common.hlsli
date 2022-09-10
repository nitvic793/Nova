struct VertexInput
{
	float3 Position : POSITION;
    float2 UV		: TEXCOORD;
	float3 Normal	: NORMAL;
	float3 Tangent	: TANGENT0;
};

struct PixelInput
{
	float4 Position		: SV_POSITION;
	float2 UV			: TEXCOORD;
	float3 Normal		: NORMAL;
	float3 Tangent		: TANGENT;
	float3 WorldPos		: POSITION0;
};

struct ObjectData
{
    float4x4 World;
};

struct FrameData
{
    float4x4 View;
    float4x4 Projection;
};
