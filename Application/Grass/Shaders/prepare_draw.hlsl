#include "grass.h"

struct IndirectArguments
{
	// Push constants
	float2 PatchOffset;
	float2 PatchScale;

	// Draw data
	uint IndexCountPerInstance;
	uint InstanceCount;
	uint StartIndexLocation;
	uint BaseVertexLocation;
	uint StartInstanceLocation;
};

struct GrassPatchDataSB
{
	uint InstanceDataOffset;
};

struct GrassPerfSettingsCB
{
	float LowpolyTreshold;
	float InstanceReductionFactor;
	uint MinInstancesPerPatch;
};

cbuffer Constants : register(b0)
{
	float4 CameraFrustum[6];
	PlaneParamsCB PlaneParams;
	uint GrassPatchSubdivision;
	uint MaxPatchInstances;
	uint HighPolyIndexCount;
	uint LowPolyIndexCount;
	float3 CameraPosition;
	GrassPerfSettingsCB GrassPerfSettings;
}

StructuredBuffer<GrassPatchDataSB> GrassPatchData : register(t0);

RWStructuredBuffer<IndirectArguments> IndArgsHighPoly : register(u0);
RWStructuredBuffer<uint> IndArgsHighPolyCount : register(u1);

RWStructuredBuffer<IndirectArguments> IndArgsLowPoly : register(u2);
RWStructuredBuffer<uint> IndArgsLowPolyCount : register(u3);

[numthreads(128,1,1)]
void CS(uint3 threadID : SV_DispatchThreadID)
{
	const uint patchIndex = threadID.x;
	if (patchIndex > GrassPatchSubdivision * GrassPatchSubdivision) return;

	const uint2 patchCoords = uint2(patchIndex / GrassPatchSubdivision, patchIndex % GrassPatchSubdivision);

	const float patchStep = 1.0f / GrassPatchSubdivision;
	const float2 patchScale = float2(patchStep, patchStep);
	const float2 patchOffset = float2(patchCoords) * patchScale;
	const float2 localPosition = patchOffset - 0.5f + patchScale / 2.0f;

	float3 bsCenter = PlaneParams.Position + float3(localPosition.x, 0.0f, localPosition.y) * PlaneParams.Scale;
	float bsRadius = max(patchStep * PlaneParams.Scale.x, max(PlaneParams.Scale.y, patchStep * PlaneParams.Scale.z));

	for (uint i = 0; i < 6; i++)
	{
		const float signedDistance = dot(float4(bsCenter, 1.0f), CameraFrustum[i]);
		if (signedDistance < -bsRadius)
			return; // Patch not visible in frustum, do not process it
	}

	float patchDistance = length(bsCenter - CameraPosition) - bsRadius;
	patchDistance = max(CameraPosition.y - PlaneParams.Position.y - PlaneParams.Scale.y, patchDistance);

	IndirectArguments args;
	args.PatchOffset = patchOffset;
	args.PatchScale = patchScale;
	args.StartInstanceLocation = GrassPatchData[patchIndex].InstanceDataOffset;
	args.StartIndexLocation = 0;
	args.BaseVertexLocation = 0;

	// Calculate instance count based on distance
	const float instanceReduction = max(1.0f, patchDistance * GrassPerfSettings.InstanceReductionFactor);
	args.InstanceCount = MaxPatchInstances;
	args.InstanceCount = args.InstanceCount / instanceReduction;
	args.InstanceCount = max(GrassPerfSettings.MinInstancesPerPatch, args.InstanceCount);

	// Select LOD level
	if (patchDistance > GrassPerfSettings.LowpolyTreshold) // Low poly
	{
		args.IndexCountPerInstance = LowPolyIndexCount;

		uint writeOffset;
		const uint writeCount = WaveActiveSum(1);
		if (WaveIsFirstLane())
		{
			InterlockedAdd(IndArgsLowPolyCount[0], writeCount, writeOffset);
		}
		writeOffset = WaveReadLaneFirst(writeOffset);
		IndArgsLowPoly[writeOffset + WavePrefixSum(1)] = args;
	}
	else // High poly
	{
		args.IndexCountPerInstance = HighPolyIndexCount;

		uint writeOffset;
		const uint writeCount = WaveActiveSum(1);
		if (WaveIsFirstLane())
		{
			InterlockedAdd(IndArgsHighPolyCount[0], writeCount, writeOffset);
		}
		writeOffset = WaveReadLaneFirst(writeOffset);
		IndArgsHighPoly[writeOffset + WavePrefixSum(1)] = args;
	}
}