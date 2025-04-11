#include "../../Common/common_shader.h"

cbuffer Constants : register(b0)
{
	Camera MainCamera;
	Camera ShadowCamera;
	float3 DirLight;
}

SamplerState s_DepthSampler : register(s0);
Texture2D<float> Depth : register(t0);
Texture2D<float> ShadowMap : register(t1);

struct VertOUT
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
};

VertOUT VS(float2 Position : SV_POSITION, float2 UV : TEXCOORD)
{
	VertOUT OUT;
	OUT.Position = float4(Position, 0.0f, 1.0f);
	OUT.UV = UV;
	return OUT;
}

#define G_SCATTERING 0.4f
#define PI 3.1415f
#define NB_STEPS 100
#define LIGHT_COLOR float3(4.0f, 4.0f, 4.0f)

// Mie scaterring approximated with Henyey-Greenstein phase function.
float ComputeScattering(float lightDotView)
{
	float result = 1.0f - G_SCATTERING * G_SCATTERING;
	result /= (4.0f * PI * pow(1.0f + G_SCATTERING * G_SCATTERING - (2.0f * G_SCATTERING) * lightDotView, 1.5f));
	return result;
}

float4 PS(VertOUT IN) : SV_Target
{
	const float depth = Depth.Sample(s_DepthSampler, IN.UV);	
	const float3 worldPos = GetWorldPositionFromDepth(depth, IN.UV, MainCamera);

	const float3 startPosition = MainCamera.Position;
	const float3 endPosition = worldPos;

	const float3 rayVector = endPosition - startPosition;
	const float rayLength = length(rayVector);
	const float3 rayDir = rayVector / rayLength;

	const float stepLength = rayLength / NB_STEPS;
	const float3 step = rayDir * stepLength;

	float3 currentPosition = startPosition;
	float3 fog = 0.0f;

	for (int i = 0; i < NB_STEPS; i++)
	{
		const float depthBias = 0.001f;
		const float4 shadowCameraSpace = GetClipPosition(currentPosition, ShadowCamera);
		const float shadowCameraDepth = shadowCameraSpace.z / shadowCameraSpace.w;
		const float2 shadowUV = GetUVFromClipPosition(shadowCameraSpace);
		const bool inShadowMap = shadowUV.x < 1.0f && shadowUV.x > 0.0f && shadowUV.y < 1.0f && shadowUV.y > 0.0f;
		float shadowValue = 1.0f;
		if (inShadowMap)
		{
			shadowValue = depthBias + ShadowMap.Sample(s_DepthSampler, shadowUV);
		}
		
		if (shadowValue > shadowCameraDepth)
		{
			fog += ComputeScattering(dot(rayDir, DirLight)).xxx * LIGHT_COLOR;
		}
		currentPosition += step;
	}
	fog /= NB_STEPS;

	return float4(fog, 0.1f);
}