#include "../../Common/common_shader.h"

struct PBRSettingsCB
{
	float3 SubsurfaceAlbedo; float Padding0;
	float3 LightDir_LightPos; float Padding1;
	float3 LightColor; float Padding2;
	float3 F0; float Padding3;
	float3 F90; float Padding4;
	float P; float3 Padding5;
};

struct VertexIN
{
	float3 Position : SV_POSITION;
	float3 Normal	: NORMAL;
};

struct VertexOUT
{
	float4 Position : SV_POSITION;
	float3 WorldPosition : WORLD_POSITION;
	float3 Normal : NORMAL;
};

cbuffer Constants : register(b0)
{
	PBRSettingsCB PBRSettings;
	Camera MainCamera;
	float4x4 ModelToWorld;
}

VertexOUT VS(VertexIN IN)
{
	const float4 worldPos = mul(float4(IN.Position, 1.0f), ModelToWorld);
	const float3 worldNormal = mul(IN.Normal, (float3x3) ModelToWorld);

	VertexOUT OUT;
	OUT.Position = GetClipPosition(worldPos.xyz, MainCamera);
	OUT.WorldPosition = worldPos.xyz;
	OUT.Normal = worldNormal;
	return OUT;
}

#ifdef BRDF_Lambert

float3 BRDF(float3 l, float3 v)
{
	return PBRSettings.SubsurfaceAlbedo / PI;
}

#else

float3 BRDF(float3 l, float3 v)
{
	return 1.0f;
}

#endif

#ifdef Illumination_Directional

float3 GetL(float3 p)
{
	return normalize(PBRSettings.LightDir_LightPos);
}

float3 Illumination(float3 n, float3 l, float3 p)
{
	return PI * max(dot(n, l), 0.0f) * PBRSettings.LightColor;
}

#elif defined(Illumination_Point)

float3 GetL(float3 p)
{
	return normalize(PBRSettings.LightDir_LightPos - p);
}

float3 Illumination(float3 n, float3 l, float3 p)
{
	const float dist = length(PBRSettings.LightDir_LightPos - p);
	return PI * max(dot(n, l), 0.0f) * PBRSettings.LightColor * (1.0f / (dist * dist));
}

#else

float3 Illumination(float3 n, float3 l, float3 p)
{
	return 1.0f;
}

float3 GetL(float3 p)
{
	return normalize(PBRSettings.LightDir_LightPos);
}

#endif

#ifdef Fresnel_Shlick

// TODO: Internal reflection

float3 Fresnel(float3 n, float3 l)
{
	const float x = pow((1.0f - max(dot(n, l), 0.0f)), (1.0f / PBRSettings.P));
	const float3 fresnelExternal = PBRSettings.F0 + (PBRSettings.F90 - PBRSettings.F0) * x;
	return fresnelExternal;
}

#else

float3 Fresnel(float3 n, float3 l)
{
	return 1.0f;
}

#endif

float4 PS(VertexOUT IN) : SV_TARGET
{
	const float3 eye = MainCamera.Position;
	const float3 p = IN.WorldPosition;
	const float3 l = GetL(p);
	const float3 v = normalize(eye - p);
	const float3 n = IN.Normal;

	const float3 brdf = BRDF(l, v);
	const float3 illumination = Illumination(n, l, p);
	const float3 fresnel = Fresnel(n, v);

	const float3 color = brdf * illumination * fresnel;

	return float4(color, 1.0f);
}