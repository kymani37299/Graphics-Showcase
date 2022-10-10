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

cbuffer Constants : register(b0)
{
	Camera MainCamera;
	float4x4 ModelToWorld;
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

float2 GetGrassPlaneUV(float3 position)
{
	return (((position - PlaneParams.Position) / PlaneParams.Scale) + 0.5f).xz;
}

void ApplyWind(in float3 grassPosition, in float3 localHeight, inout float3 vertexPosition)
{
	const float3 WindDirection = float3(1.0f, 0.0f, 0.0f);

	const float2 planeUV = GetGrassPlaneUV(grassPosition);
	const float windInfluence = GetWindInfluence(planeUV);
	const float3 windOffset = lerp(float3(0.0f, 0.0f, 0.0f), WindDirection, localHeight * localHeight);
	const float windStrength = 2.0f;
	vertexPosition += WindDirection * windOffset * windInfluence * windStrength;
}

VertexOUT VS(VertexIN IN)
{
	const GrassInstance instanceData = GrassInstanceBuffer[IN.InstanceID];
	const float localHeight = 1.0f - IN.Texcoord.g;

	const float2 grassPlaneUV = instanceData.Position + 0.5f;
	const float terrainHeight = HeightTexture.SampleLevel(s_LinearWrap, grassPlaneUV, 0).r;
	
	const float3 grassPosition = PlaneParams.Position + PlaneParams.Scale * float3(instanceData.Position.x, terrainHeight, instanceData.Position.y);
	float3 vertexPosition = IN.Position + float3(0.0f, 0.0f, -1.0f); // Hack: Not sure why I need this adding
	vertexPosition.z *= instanceData.Height;
	float3 worldPos = mul(vertexPosition, (float3x3)ModelToWorld); // Intern transformations
	worldPos = mul(worldPos, GetRotation(instanceData.Normal));
	worldPos = worldPos + grassPosition; // Instance position
	ApplyWind(grassPosition, localHeight, worldPos); // Wind
	
	VertexOUT OUT;
	OUT.Position = GetClipPosition(worldPos, MainCamera);
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