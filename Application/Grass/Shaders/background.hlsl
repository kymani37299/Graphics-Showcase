cbuffer Constants : register(b0)
{
	float3 Color;
}

float4 VS(float2 Position : SV_POSITION, float2 UV : TEXCOORD) : SV_POSITION
{
	return float4(Position, 0.0f, 1.0f);
}

float4 PS(float4 Position : SV_POSITION) : SV_Target
{
	return float4(Color, 1.0f);
}