#include "../../Common/common_shader.h"
#include "grass.h"

struct VertexIN
{
	float2 Position : SV_POSITION;
};

struct VertexOUT
{
	float4 Position : SV_POSITION;
};

cbuffer Constants : register(b0)
{
	Camera MainCamera;
	PlaneParamsCB PlaneParams;
	float3 FogColor;
}

SamplerState s_LinearWrap : register(s0);
Texture2D<float4> HeightMap : register(t0);

VertexOUT VS(VertexIN IN)
{
	const float2 planeUV = IN.Position + 0.5f;
	const float height = HeightMap.SampleLevel(s_LinearWrap, planeUV, 0).r;

	const float3 modelPos = float3(IN.Position.x, height, IN.Position.y);
	const float3 worldPos = modelPos * PlaneParams.Scale + PlaneParams.Position;

	VertexOUT OUT;
	OUT.Position = GetClipPosition(worldPos, MainCamera);
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
	float3 finalColor = PlaneParams.Color;
	ApplyFog(finalColor, IN.Position.z / IN.Position.w);
	return float4(finalColor, 1.0f);
}