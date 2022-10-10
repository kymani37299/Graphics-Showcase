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

float4 PS(VertexOUT IN) : SV_Target
{
	return float4(PlaneParams.Color, 1.0f);
}