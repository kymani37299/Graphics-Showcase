#include "../../Common/common_shader.h"
#include "grass.h"

struct GrassSettingsCB
{
	float3 TipColor; float Pad0;
	float3 TopColor; float Pad1;
	float3 BottomColor; float Pad2;
	float3 AmbientOcclusionColor; float Pad3;

	float2 TipRange; 
	float2 AmbientOcclusionRange;
};

struct GrassInstance
{
	float2 Position;
	float3 Normal;
	float Height;
};

struct VertexIN
{
	float3 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD;
	uint InstanceID : SV_InstanceID;
};

struct VertexOUT
{
	float4 Position : SV_POSITION;
	float LocalHeight : GRASS_HEGHT; // [0,1] - 0 root, 1 tip
};

cbuffer PushConstants : register(b128)
{
	float2 PatchOffset;
	float2 PatchScale;
};

cbuffer Constants : register(b0)
{
	Camera MainCamera;
	GrassSettingsCB GrassSettings;
	PlaneParamsCB PlaneParams;
	float3 FogColor;
}

SamplerState s_LinearWrap : register(s0);

StructuredBuffer<GrassInstance> GrassInstanceBuffer : register(t0);
Texture2D<float4> WindTexture : register(t1);
Texture2D<float4> HeightTexture : register(t2);

float3x3 GetRotation(float3 forward)
{
	const float3 up = float3(0.0f, 1.0f, 0.0f);
	const float3 axis_x = normalize(cross(up, forward));
	const float3 axis_y = normalize(cross(forward, axis_x));
	return float3x3(axis_x, axis_y, forward);
}

float GetWindInfluence(float2 planeUV)
{
	const float noiseValue = WindTexture.SampleLevel(s_LinearWrap, planeUV, 0).r;
	return 2.0f * noiseValue - 1.0f;
}

float2 GetGrassPlaneUV(float2 instancePosition)
{
	return (((instancePosition - PlaneParams.Position.xz) / PlaneParams.Scale.xz) + 0.5f);
}

float3 AnimateGrass(const float3 vertexPosition, const float localHeight, const float2 planeUV)
{
	const float3 WindDirection = float3(1.0f, 0.0f, 0.0f);

	const float windInfluence = GetWindInfluence(planeUV);
	const float3 windOffset = lerp(float3(0.0f, 0.0f, 0.0f), WindDirection, localHeight * localHeight);
	const float windStrength = 2.0f;
	return vertexPosition + WindDirection * windOffset * windInfluence * windStrength;
}

float2 GetGrassInstancePosition(const GrassInstance instanceData)
{
	const float2 patchPosition = PatchOffset + instanceData.Position * PatchScale - 0.5f + PatchScale/2.0f;
	const float2 worldPosition = PlaneParams.Position.xz + PlaneParams.Scale.xz * patchPosition;
	return worldPosition;
}

float3 GetGrassModelPosition(const float3 vertexPosition, const GrassInstance instanceData)
{
	// Not sure why I need to add -1.0f
	float3 modelPosition = vertexPosition + float3(0.0f, 0.0f, -1.0f);
	modelPosition = mul(modelPosition, GetRotation(instanceData.Normal));
	modelPosition.y *= instanceData.Height;
	return modelPosition;
}

VertexOUT VS(VertexIN IN)
{
	const GrassInstance instanceData = GrassInstanceBuffer[IN.InstanceID];

	const float2 instancePosition = GetGrassInstancePosition(instanceData);
	const float2 grassPlaneUV = GetGrassPlaneUV(instancePosition);
	const float terrainHeight = HeightTexture.SampleLevel(s_LinearWrap, grassPlaneUV, 0).r;

	const float localHeight = 1.0f - IN.Texcoord.g;
	const float3 modelPosition = GetGrassModelPosition(IN.Position, instanceData);
	const float3 animatedModelPosition = AnimateGrass(modelPosition, localHeight, grassPlaneUV);

	float3 worldPosition = animatedModelPosition;
	worldPosition += float3(instancePosition.x, 0.0f, instancePosition.y);
	worldPosition.y += terrainHeight * PlaneParams.Scale.y;

	VertexOUT OUT;
	OUT.Position = GetClipPosition(worldPosition, MainCamera);
	OUT.LocalHeight = localHeight;
	return OUT;
}

void ApplyFog(inout float3 color, in float depth)
{
	const float FogStart = 0.98f;
	const float FogEnd = 1.0f;

	const float distance = 1.0f - depth;
	const float fogFactor = smoothstep(FogStart, FogEnd, distance);

	color = lerp(color, FogColor, fogFactor);
}

float4 PS(VertexOUT IN) : SV_Target
{
	const float localHeight = IN.LocalHeight; // Normalized to [0,1]
	const float3 baseColor = lerp(GrassSettings.BottomColor, GrassSettings.TopColor, localHeight);
	
	const float aoFactor = smoothstep(GrassSettings.AmbientOcclusionRange.x, GrassSettings.AmbientOcclusionRange.y, localHeight);
	const float tipFactor = smoothstep(GrassSettings.TipRange.x, GrassSettings.TipRange.y, localHeight);

	float3 finalColor = lerp(baseColor, GrassSettings.AmbientOcclusionColor, aoFactor) + tipFactor * GrassSettings.TipColor;
	ApplyFog(finalColor, IN.Position.z / IN.Position.w);

	return float4(finalColor, 1.0f);
}