#pragma once

#include <Engine/Common.h>

struct GrassSettingsCB
{
	DirectX::XMFLOAT3A GetColor(uint32_t r, uint32_t g, uint32_t b) { return (Float3{ (float) r, (float) g,(float) b } / Float3{ 255.0f, 255.0f, 255.0f }).ToXMFA(); }

	DirectX::XMFLOAT3A TipColor = GetColor(245, 240, 187);
	DirectX::XMFLOAT3A TopColor = GetColor(196, 223, 170);
	DirectX::XMFLOAT3A BottomColor = GetColor(144, 200, 172);
	DirectX::XMFLOAT3A AmbientOcclusionColor = GetColor(115, 169, 173);

	DirectX::XMFLOAT2 TipRange{0.7f, 2.0f};
	DirectX::XMFLOAT2 AmbientOcclusionRange{0.7f, 0.0f};
};

struct GrassPerfSettingsCB
{
	float LowpolyTreshold = 50.0f;
	float InstanceReductionFactor = 0.15f;
	uint32_t MinInstancesPerPatch = 50;
};

struct GrassGenerationConfiguration
{
	uint32_t NumInstances = 50000000;
	Float2 HeightRange = { 0.7f, 1.4f };
	Float3 PlanePosition = { 0.0f, 0.0f, 0.0f };
	Float3 PlaneScale = { 1000.0f, 20.0f, 1000.0f };
};

extern GrassSettingsCB GrassSettings;
extern GrassPerfSettingsCB GrassPerfSettings;
extern GrassGenerationConfiguration GrassGenConfig;