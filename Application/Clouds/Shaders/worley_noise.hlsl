cbuffer Constants : register(b0)
{
	uint NumPoints;
	float3 NoiseDimensions;
	float NormalizationFactor;
};

struct SamplePoint
{
	float3 Samples[4];
};

StructuredBuffer<SamplePoint> Points : register(t0);
RWTexture3D<float4> OutputTexture : register(u0);

float SquaredDistance(in float3 a, in float3 b)
{
	const float diffX = a.x - b.x;
	const float diffY = a.y - b.y;
	const float diffZ = a.z - b.z;
	return diffX * diffX + diffY * diffY + diffZ * diffZ;
}

static const float MAX_DIST = 10000.0f;

float4 GetDistanceFromPoint(float3 position, SamplePoint p)
{
	float4 minDistance = MAX_DIST;
	for (float i = -1.0f; i <= 1.0f; i++)
	{
		for (float j = -1.0f; j <= 1.0f; j++)
		{
			for (float k = -1.0f; k <= 1.0f; k++)
			{
				for (uint l = 0; l < 4; l++)
				{
					float pointDist = SquaredDistance(position, p.Samples[l] + float3(i, j, k));
					minDistance[l] = min(pointDist, minDistance[l]);
				}
			}
		}
	}
	return minDistance;
}

float4 GetVoronoiSquaredDistance(float3 position)
{
	float4 minDistance = MAX_DIST;
	for (uint i = 0; i < NumPoints; i++)
	{
		const float4 pointDist = GetDistanceFromPoint(position, Points[i]);
		minDistance = min(pointDist, minDistance);
	}
	return minDistance;
}

[numthreads(8, 8, 8)]
void CS(uint3 threadID : SV_DispatchThreadID)
{
	const uint3 pixelCoord = threadID.xyz;
	const float3 pixelPos = pixelCoord / NoiseDimensions;

	const float4 minDistance = GetVoronoiSquaredDistance(pixelPos);
	OutputTexture[pixelCoord] = 1.0f - sqrt(minDistance * NormalizationFactor);
}