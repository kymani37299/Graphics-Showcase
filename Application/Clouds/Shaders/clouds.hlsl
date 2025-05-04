#include "../../Common/common_shader.h"

struct CloudsSettingsCB
{
	// Cloud box
	float3 Position; float Pad0;
	float3 Size; float Pad1;

	// Sampling noise
	float4 SamplingWeights;
	float4 SamplingDetailWeights;
	float SamplingScale;
	float SamplingOffset;
	float SamplingDetailScale;
	float SamplingDetailOffset;

	// Cloud density
	float DensityTreshold;
	float DensityMultiplier;
	float DetailDensityMultiplier;

	// Light absorbtion
	float CloudLightAbsorption;
	float SunLightAbsorption;
	float SunLightBias;
	float SunPhaseValue;

	// Raymach settings
	float CloudMarchStepSize;
	float LightMarchStepSize;
};

struct SunSettingsCB
{
	float3 Position; float Pad0;
	float3 Radiance; float Pad1;
};

struct CloudBox
{
	float3 BoundsMin;
	float3 BoundsMax;
};

CloudBox CreateCloudBox(float3 position, float3 scale)
{
	CloudBox box;
	box.BoundsMin = position - scale / 2.0f;
	box.BoundsMax = position + scale / 2.0f;
	return box;
}

struct VertOUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer Constants : register(b0)
{
	CloudsSettingsCB CloudSettings;
	SunSettingsCB SunSettings;
	Camera MainCamera;
}

SamplerState s_LinearWrap : register(s0);
Texture3D<float4> CloudNoise : register(t0);
Texture3D<float4> CloudDetailNoise : register(t1);

VertOUT VS(float2 pos : SV_POSITION, float2 uv : TEXCOORD)
{
	VertOUT OUT;
	OUT.pos = float4(pos, 0.0f, 1.0f);
	OUT.uv = uv;
	return OUT;
}

struct Ray
{
	float3 Origin;
	float3 Direction;
};

struct RaytraceResult
{
	bool Hit;
	float DistanceNear;
	float DistanceFar;
};

RaytraceResult Raytrace(Ray ray, CloudBox box)
{
	const float3 t0 = (box.BoundsMin - ray.Origin) / ray.Direction;
	const float3 t1 = (box.BoundsMax - ray.Origin) / ray.Direction;
	
	const float3 tmin = min(t0, t1);
	const float3 tmax = max(t0, t1);

	const float dstA = max(max(tmin.x, tmin.y), tmin.z);
	const float dstB = min(min(tmax.x, tmax.y), tmax.z);

	RaytraceResult result;
	result.DistanceNear = max(0.0f, dstA);
	result.DistanceFar = max(0.0f, dstB);
	result.Hit = (dstB - result.DistanceNear) > 0.0f;
	return result;
}

// Remaps range (srcA,srcB) to (dstA, dstB) in a moment t
float remap(float srcA, float srcB, float dstA, float dstB, float t) 
{
	return lerp(dstA, dstB, (t - srcA) / (srcB - srcA));
}

static const float EdgeFadeDistance = 50.0f;
static const float MinHeightGradient = 0.2f;
static const float MaxHeightGradient = 0.7f;

float SampleCloudDensity(float3 position)
{
	const CloudBox cloudBounds = CreateCloudBox(CloudSettings.Position, CloudSettings.Size);

	// Height gradient
	const float heightPercent = (position.y - cloudBounds.BoundsMin.y) / CloudSettings.Size.y;
	const float heightGradient = saturate(remap(0.0, MinHeightGradient, 0, 1, heightPercent)) * saturate(remap(1, MaxHeightGradient, 0, 1, heightPercent));

	// Edge gradient
	float edgeDistance = min(position.x - cloudBounds.BoundsMin, cloudBounds.BoundsMax - position.x);
	edgeDistance = min(edgeDistance, min(position.z - cloudBounds.BoundsMin, cloudBounds.BoundsMax - position.z));
	const float edgeGradient = min(1.0f, edgeDistance / EdgeFadeDistance);
	
	// Cloud shape
	const float gradientEffect = heightGradient * edgeGradient;
	const float3 shapeSamplePos = position * CloudSettings.SamplingScale * 0.01f + CloudSettings.SamplingOffset;
	const float4 shapeNoise = CloudNoise.SampleLevel(s_LinearWrap, shapeSamplePos, 0);
	const float shapeFBM = dot(shapeNoise, CloudSettings.SamplingWeights);
	const float shapeDensity = max(0.0f, shapeFBM - CloudSettings.DensityTreshold) * gradientEffect;

	if (shapeDensity > 0.0f)
	{
		const float3 detailSamplePos = shapeSamplePos * CloudSettings.SamplingDetailScale + CloudSettings.SamplingDetailOffset;
		const float4 detailNoise = CloudDetailNoise.SampleLevel(s_LinearWrap, detailSamplePos, 0);
		const float detailFBM = dot(detailNoise, CloudSettings.SamplingDetailWeights);

		const float inverseShapeFBM = 1.0f - shapeFBM;
		const float detailMask = inverseShapeFBM * inverseShapeFBM;
		const float detailDensity = (detailFBM - 0.85f) * detailMask * gradientEffect * CloudSettings.DetailDensityMultiplier;

		return (shapeDensity + detailDensity) * CloudSettings.DensityMultiplier;
	}

	return 0.0f;
}

float GetTransmittance(float density)
{
	return clamp(exp(-density), 0.0f, 1.0f);
}

// Calculate proportion of the light that reaches given point from lightsource
float LightMarch(float3 position)
{
	const CloudBox cloudBounds = CreateCloudBox(CloudSettings.Position, CloudSettings.Size);
	const float3 toLight = normalize(SunSettings.Position - position);

	Ray ray;
	ray.Origin = position;
	ray.Direction = toLight;
	RaytraceResult result = Raytrace(ray, cloudBounds);
	// ASSERT(result.Hit, "It must hit since we are inside of a box");

	float boxDistance = 0.0f;
	float3 samplePos = position;

	float totalDensity = 0.0f;
	while(boxDistance < result.DistanceFar)
	{
		totalDensity += SampleCloudDensity(samplePos) * CloudSettings.LightMarchStepSize;
		samplePos += toLight * CloudSettings.LightMarchStepSize;
		boxDistance += CloudSettings.LightMarchStepSize;
		// boxDistance += 10.0f;
	}

	const float transmittance = GetTransmittance(totalDensity * CloudSettings.SunLightAbsorption);
	return CloudSettings.SunLightBias + transmittance * (1.0f - CloudSettings.SunLightBias);
}

// x - transmittance, y - light energy
float2 CloudMarch(Ray ray, RaytraceResult cloudboxResult)
{
	float transmittance = 1.0f;
	float lightEnergy = 0.0f;

	float boxDistance = cloudboxResult.DistanceNear;
	float3 samplePos = ray.Origin + ray.Direction * cloudboxResult.DistanceNear;
	while(boxDistance < cloudboxResult.DistanceFar)
	{
		const float density = SampleCloudDensity(samplePos);
		if (density > 0.0f)
		{
			const float lightTransmittance = LightMarch(samplePos);
			lightEnergy += density * CloudSettings.CloudMarchStepSize * transmittance * lightTransmittance * CloudSettings.SunPhaseValue;
			transmittance *= GetTransmittance(density * CloudSettings.CloudMarchStepSize * CloudSettings.CloudLightAbsorption);
	
			// Ealy exit
			if (transmittance < 0.01f) break;
		}

		samplePos += ray.Direction * CloudSettings.CloudMarchStepSize;
		boxDistance += CloudSettings.CloudMarchStepSize;
	}

	return float2(transmittance, lightEnergy);
}

static const float4 BGColor = float4(135, 206, 235, 255) / 255.0f;

float4 PS(VertOUT IN) : SV_Target
{
	const CloudBox cloudBounds = CreateCloudBox(CloudSettings.Position, CloudSettings.Size);

	// Create screen ray
	const float2 uv = -(2.0f * IN.uv - 1.0f);
	Ray ray;
	ray.Origin = MainCamera.Position;
	ray.Direction = MainCamera.Right * uv.x + MainCamera.Up * uv.y + MainCamera.Forward;
	
	// Raytrace cloud box
	RaytraceResult result = Raytrace(ray, cloudBounds);

	// Early return if ray didn't hit the box
	if (!result.Hit) return BGColor;

	const float2 marchRes = CloudMarch(ray, result);
	const float4 cloudColor = marchRes.y * float4(SunSettings.Radiance, 1.0f);

	return BGColor * marchRes.x + cloudColor;
}