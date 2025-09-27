#include "../../Common/common_shader.h"

struct PBRSettingsCB
{
	float3 SubsurfaceAlbedo; float Padding0;
	float3 LightDir_LightPos; float Padding1;
	float3 LightColor; float Padding2;
	float3 F0; float Padding3;
	float3 F90; float Padding4;
	float P; float Roughness; float2 Padding5;
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

float pos(float x)
{
	return x > 0.0f ? 1.0f : 0.0f;
}

float GetAzimuthAngle(float3 v1, float3 v2)
{
	float2 v1_xz = normalize(float2(v1.x, v1.z));
	float2 v2_xz = normalize(float2(v2.x, v2.z));

	float angle1 = atan2(v1_xz.y, v1_xz.x);
	float angle2 = atan2(v2_xz.y, v2_xz.x);

	float azimuth = angle2 - angle1;

	if (azimuth < 0.0)
		azimuth += 2.0 * PI;

	return azimuth;
}

float GetLambdaA(float3 n, float3 s)
{
	const float a = dot(n, s);
	const float r = PBRSettings.Roughness;
	const float b = r * sqrt(1.0f - a * a);
	return a / b;
}

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

#ifdef NDF_Beckmann

float Lambda(float a)
{
	if (a >= 1.6f)
		return 0.0f;

	const float a2 = a * a;
	const float x = 1.0f - 1.259f * a + 0.396f * a2;
	const float y = 3.535f * a + 2.181f * a2;

	return x / y;
}

float3 NDF(float3 n, float3 m)
{
	const float x = dot(n, m);
	const float x2 = x * x;
	const float r2 = PBRSettings.Roughness * PBRSettings.Roughness;

	const float a = pos(x) / (PI * r2 * x2 * x2);
	const float b = (x2 - 1.0f) / (r2 * x2);

	return a * exp(b);
}

#elif defined(NDF_BlinnPhong)

float Lambda(float a)
{
	return 1.0f; // TODO: Implement
}

float3 NDF(float3 n, float3 m)
{
	const float x = dot(n, m);
	const float r = PBRSettings.Roughness;
	return pos(x) * (r + 2.0f) / (2.0f * PI) * pow(x, r);
}

#elif defined(NDF_GGX)

float Lambda(float a)
{
	const float a2 = a * a;
	return (-1.0f + sqrt(1.0f + (1.0f / a2))) / 2.0f;
}

float3 NDF(float3 n, float3 m)
{
	const float r = PBRSettings.Roughness;
	const float r2 = r * r;
	const float x = dot(n, m);

	const float a = pos(x) * r2;
	const float b = 1.0f + x * x * (r2 - 1.0f);
	return a / (PI * b * b);
}

#else

float Lambda(float a)
{
	return 1.0f;
}

float3 NDF(float3 n, float3 m)
{
	return 1.0f;
}

#endif

float G1(float3 m, float3 v)
{
	const float a = GetLambdaA(m, v);
	return pos(dot(m, v)) / (1.0f + Lambda(a));
}

#ifdef MaskingShadowing_Smith

float3 MaskingShadowing(float3 l, float3 v, float3 m)
{
	return G1(v, m) * G1(l, m);
}

#elif defined(MaskingShadowing_SmithAngleFix1)

float3 MaskingShadowing(float3 l, float3 v, float3 m)
{
	const float azimuth = GetAzimuthAngle(v, l);
	const float f = 1.0f - exp(-7.3f * azimuth);

	const float a = G1(v, m);
	const float b = G1(l, m);

	return f * a * b + (1.0f - f) * min(a, b);
}

#elif defined(MaskingShadowing_SmithAngleFix2)

float3 MaskingShadowing(float3 l, float3 v, float3 m)
{
	const float azimuth = GetAzimuthAngle(v, l);

	const float c = 4.41 * azimuth;
	const float f = c / (c + 1.0f);

	const float a = G1(v, m);
	const float b = G1(l, m);

	return f * a * b + (1.0f - f) * min(a, b);
}

#elif defined(MaskingShadowing_SmithHeightCorrelated)

float3 MaskingShadowing(float3 l, float3 v, float3 m)
{
	const float lav = GetLambdaA(m, v);
	const float lal = GetLambdaA(m, l);

	const float a = pos(dot(m, v)) * pos(dot(m, l));
	const float b = 1.0f + Lambda(lav) + Lambda(lal);
	return a / b;
}

#elif defined(MaskingShadowing_Heitz)

float3 MaskingShadowing(float3 l, float3 v, float3 m)
{
	const float a = pos(dot(m, v)) * pos(dot(m, l));
	const float lav = GetLambdaA(m, v);
	const float lal = GetLambdaA(m, l);
	const float lv = Lambda(lav);
	const float ll = Lambda(lal);

	const float azimuth = GetAzimuthAngle(v, l);
	const float empiricalFunction = 1.0f - exp(-7.3f * azimuth); // Empirical function, find something better, I used one from angle fix
	const float b = 1.0f * max(lv, ll) + empiricalFunction * min(lv, ll);

	return a / b;
}

#else

float3 MaskingShadowing(float3 l, float3 v, float3 m)
{
	return 1.0f;
}

#endif

#ifdef BRDF_Lambert

float3 BRDF(float3 l, float3 v, float3 n)
{
	const float3 fresnel = Fresnel(n, v);
	return fresnel * PBRSettings.SubsurfaceAlbedo / PI;
}

#elif defined(BRDF_PBR)

float3 BRDF(float3 l, float3 v, float3 n)
{
	const float3 h = normalize(l + v);
	const float3 fresnel = Fresnel(h, v);
	const float3 ndf = NDF(n, h);
	const float3 maskingShadowing = MaskingShadowing(l, v, h);

	return (fresnel * maskingShadowing * ndf) / (4 * dot(n, l) * dot(n, v));
}

#else

float3 BRDF(float3 l, float3 v, float3 n)
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

float4 PS(VertexOUT IN) : SV_TARGET
{
	const float3 eye = MainCamera.Position;
	const float3 p = IN.WorldPosition;
	const float3 l = GetL(p);
	const float3 v = normalize(eye - p);
	const float3 n = IN.Normal;

	const float3 brdf = BRDF(l, v, n);
	const float3 illumination = Illumination(n, l, p);

	const float3 color = brdf * illumination * dot(n,l);

	return float4(color, 1.0f);
}