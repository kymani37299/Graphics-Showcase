#include "../../Common/common_shader.h"

cbuffer SceneConstants : register(b0)
{
	Camera MainCamera;
}

cbuffer ObjectConstants : register(b1)
{
	float4x4 ModelToWorld;
}

float4 VS(float3 Position : POSITION) : SV_POSITION
{
	const float3 worldPosition = mul(float4(Position, 1.0f), ModelToWorld).xyz;
	return GetClipPosition(worldPosition, MainCamera);
}