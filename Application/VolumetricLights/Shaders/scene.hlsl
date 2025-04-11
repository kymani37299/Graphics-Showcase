#include "../../Common/common_shader.h"

cbuffer SceneConstants : register(b0)
{
	Camera MainCamera;
	Camera ShadowCamera;
	float3 DirLight;
}

cbuffer ObjectConstants : register(b1)
{
	float4x4 ModelToWorld;
	float4x4 ModelToWorldNormal;
	float3 Color;
}

struct VertexIN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

struct VertexOUT
{
	float4 Position : SV_POSITION;
	float3 WorldPos : WORLD_POS;
	float3 Normal : NORMAL;
};

SamplerState s_ShadowSampler : register(s0);
Texture2D<float> ShadowMap : register(t0);

VertexOUT VS(VertexIN IN)
{
	const float3 worldPosition = mul(float4(IN.Position, 1.0f), ModelToWorld).xyz;
	const float3 worldNormal = mul(float4(IN.Normal, 0.0f), ModelToWorldNormal).xyz;

	VertexOUT OUT;
	OUT.Position = GetClipPosition(worldPosition, MainCamera);
	OUT.WorldPos = worldPosition;
	OUT.Normal = worldNormal;
	return OUT;
}

float CalculateShadowFactor(float3 worldPosition)
{
	const float4 shadowmapClip = GetClipPosition(worldPosition, ShadowCamera);
	const float2 shadowmapUV = GetUVFromClipPosition(shadowmapClip);

	const bool inShadowMap = shadowmapUV.x < 1.0f && shadowmapUV.x > 0.0f && shadowmapUV.y < 1.0f && shadowmapUV.y > 0.0f;
	float shadowFactor = 1.0f;
	if (inShadowMap)
	{
		const float depthBias = 0.02f;
		const float shadowmapDepth = ShadowMap.Sample(s_ShadowSampler, shadowmapUV) + depthBias;
		const bool isInShadow = shadowmapClip.z > shadowmapDepth;
		shadowFactor = isInShadow ? 0.3f : 1.0f;
	}
	return shadowFactor;
}

float4 PS(VertexOUT IN) : SV_Target
{
	return float4(Color, 1.0f) * clamp(dot(DirLight, normalize(IN.Normal)), 0.0f, 1.0f);
}