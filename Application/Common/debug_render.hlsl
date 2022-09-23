#include "common_shader.h"

cbuffer Constants : register(b0)
{
	Camera MainCamera;
	float4 Color;
	float3 Position;
	float Scale;
}

float4 VS(float3 position : SV_POSITION) : SV_POSITION
{
	const float3 worldPos = position * Scale + Position;
	const float4 viewPos = mul(float4(worldPos, 1.0f), MainCamera.WorldToView);
	const float4 clipPos = mul(viewPos, MainCamera.ViewToClip);
	return clipPos;
}

float4 PS(float4 position : SV_POSITION) : SV_TARGET
{
	return Color;
}