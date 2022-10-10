#include "../Common/common_shader.h"

// Must be divisible by 4
#define MAX_MORPH_WEIGHTS 16
#define MAX_SKELETON_JOINTS 128

struct VertexIN
{
	float3 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD;
	float3 Normal	: NORMAL;

#ifdef APPLY_MORPHS
	uint VertexID	: SV_VertexID;
#endif // APPLY_MORPHS

#ifdef APPLY_SKIN
	float4 Weights	: WEIGHTS;
	uint4 Joints	: JOINTS;
#endif // APPLY_SKIN
};

struct VertexOUT
{
	float4 Position : SV_POSITION;
	float3 WorldPosition : WORLD_POS;
	float2 Texcoord : TEXCOORD;
	float3 Normal	: NORMAL;
};

struct MorphVertex
{
	float3 Position;
	float2 Texcoord;
	float3 Normal;
	float4 Tangent;
};

cbuffer Constants : register(b0)
{
	Camera MainCamera;
	float4x4 AnimationTransformation;
	float4x4 ModelToWorld;
	float3 AlbedoFactor;

#ifdef APPLY_MORPHS
	uint MorphWeightCount;
	float4 MorphWeights[MAX_MORPH_WEIGHTS / 4];
#endif // APPLY_MORPHS

#ifdef APPLY_SKIN
	float4x4 JointTransformations[MAX_SKELETON_JOINTS];
	float4x4 JointAnimationTransformations[MAX_SKELETON_JOINTS];
	float4x4 ModelToJointTransformations[MAX_SKELETON_JOINTS];
#endif // APPLY_SKIN
}

SamplerState s_LinearWrap : register(s0);
Texture2D<float4> Albedo : register(t0);

#ifdef APPLY_MORPHS
StructuredBuffer<MorphVertex> MorphTargets[MAX_MORPH_WEIGHTS] : register(t1);
#endif // APPLY_MORPHS

void ApplyMorph(inout VertexIN vertex)
{
#ifdef APPLY_MORPHS
	 for (uint i = 0; i < MorphWeightCount; i++)
	 {
	 	const float weight = MorphWeights[i/4][i%4];
	 	MorphVertex targetVert = MorphTargets[i][vertex.VertexID];
		vertex.Position += weight * targetVert.Position;
		vertex.Texcoord += weight * targetVert.Texcoord;
		vertex.Normal += weight * targetVert.Normal;
	 }
#endif // APPLY_MORPHS
}

void ApplySkin(inout VertexIN vertex)
{
#ifdef APPLY_SKIN
	float4x4 skinMatrix = 0.0f;
	[unroll]
	for (uint i = 0; i < 4; i++)
	{
		const uint jointIndex = vertex.Joints[i];
		const float4x4 toModel = mul(JointTransformations[jointIndex], JointAnimationTransformations[jointIndex]);
		const float4x4 jointTransformation = mul(ModelToJointTransformations[jointIndex], toModel);
		skinMatrix += vertex.Weights[i] * jointTransformation;
	}
	vertex.Position = mul(float4(vertex.Position, 1.0f), skinMatrix).xyz;
	vertex.Normal = mul(vertex.Normal, (float3x3) skinMatrix);
#endif // APPLY_SKIN
}


VertexOUT VS(VertexIN IN)
{
	VertexIN vertex = IN;
	ApplyMorph(vertex);
	// ApplySkin(vertex);

	vertex.Position = mul(float4(vertex.Position, 1.0f), AnimationTransformation).xyz;
	vertex.Position = mul(float4(vertex.Position, 1.0f), ModelToWorld).xyz;
	vertex.Normal = mul(vertex.Normal, (float3x3) AnimationTransformation);
	vertex.Normal = mul(vertex.Normal, (float3x3) ModelToWorld);

	VertexOUT OUT;
	OUT.Position = GetClipPosition(vertex.Position, MainCamera);
	OUT.WorldPosition = vertex.Position;
	OUT.Texcoord = vertex.Texcoord;
	OUT.Normal = vertex.Normal;
	return OUT;
}

float4 PS(VertexOUT IN) : SV_TARGET
{
	const float3 dirLightColor = float3(1.0f, 1.0f, 1.0f);
	const float3 dirLight = normalize(float3(0.8f, -0.7f, -0.8f));
	const float3 ambient = 0.1f;
	const float specularExponent = 8.0f;

	const float3 normal = normalize(IN.Normal);
	const float3 toEye = normalize(MainCamera.Position - IN.WorldPosition);

	const float4 albedoTexture = Albedo.SampleLevel(s_LinearWrap, IN.Texcoord, 0);
	const float3 albedo = AlbedoFactor * (albedoTexture.a > 0.01f ? albedoTexture.rgb : 0.7f);

	const float diffuseFactor = max(0.0f, dot(-dirLight, normal));
	const float specularFactor = max(0.0f, pow(dot(reflect(dirLight, normal), toEye), specularExponent));
	
	const float3 color = ambient + albedo * diffuseFactor + dirLightColor * specularFactor;
	return float4(color, 1.0f);
}