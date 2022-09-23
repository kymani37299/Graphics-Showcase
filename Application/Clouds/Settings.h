#pragma once

#include <Engine/Common.h>

struct CloudsSettingsCB
{
	// Cloud box
	DirectX::XMFLOAT3A Position = {50.0f, 200.0f, -50.0f};
	DirectX::XMFLOAT3A Size = {500.0f, 50.0f, 500.0f};

	// Sampling noise
	DirectX::XMFLOAT4 SamplingWeights = { 0.627f, 0.564f, 0.503f, 0.191f };
	DirectX::XMFLOAT4 SamplingDetailWeights = { 0.3f, 0.099f, 0.517f, 0.795f };
	float SamplingScale = 0.45f;
	float SamplingOffset = 0.0f;
	float SamplingDetailScale = 3.2f;
	float SamplingDetailOffset = 0.0f;

	// Cloud density
	float DensityTreshold = 0.85f;
	float DensityMultiplier = 0.15f;
	float DetailDensityMultiplier = 4.0f;
	
	// Light absorbtion
	float CloudLightAbsorption = 0.7f;
	float SunLightAbsorption = 0.75f;
	float SunLightBias = 0.3f;
	float SunPhaseValue = 0.8f;

	// Raymach num steps
	float CloudMarchNumSteps = 12.0f;
	float LightMarchNumSteps = 6.0f;
};

struct SunSettingsCB
{
	DirectX::XMFLOAT3A Position = { 10.0f, 500.0f, 10.0f };
	DirectX::XMFLOAT3A Radiance = { 1.0f, 246.0f / 255.0f, 214.0f / 255.0f };
};

struct CloudNoiseSettingsStruct
{
	uint32_t NumSamplePoints = 48;
	float NormalizationFactor = 10.0f;
	uint32_t VolumeWidth = 64;
	uint32_t VolumeHeight = 64;
	uint32_t VolumeDepth = 64;
};

extern CloudsSettingsCB CloudSettings;
extern SunSettingsCB SunSettings;
extern CloudNoiseSettingsStruct CloudNoiseSettings;