cbuffer Constants : register(b0)
{
	float2 TextureSize;
	float TimeSeconds;
};

RWTexture2D<float4> OutputTexture : register(u0);

float Rand(float2 n) 
{
	const float3 seed = float3(12.9898f, 4.1414f, 43758.5453f);
	return frac(sin(dot(n, seed.xy)) * seed.z);
}

float Noise(float2 uv)
{
	const float2 ip = floor(uv);
	float2 u = frac(uv);
	u = u * u * (3.0 - 2.0 * u);

	const float v1 = lerp(Rand(ip), Rand(ip + float2(1.0, 0.0)), u.x);
	const float v2 = lerp(Rand(ip + float2(0.0, 1.0)), Rand(ip + float2(1.0, 1.0)), u.x);
	const float res = lerp(v1, v2, u.y);
	return res * res;
}

float FBM(float2 uv)
{
	const float2x2 mtx = float2x2(0.80, 0.60, -0.60, 0.80);

	const float t = TimeSeconds * 0.2f;

	float fbm = 0.0;
	fbm += 0.500000 * Noise(uv + t); uv = mul(mtx, uv * 2.02);
	fbm += 0.031250 * Noise(uv); uv = mul(mtx,uv * 2.01);
	fbm += 0.250000 * Noise(uv); uv = mul(mtx,uv * 2.03);
	fbm += 0.125000 * Noise(uv); uv = mul(mtx,uv * 2.01);
	fbm += 0.062500 * Noise(uv); uv = mul(mtx,uv * 2.04);
	fbm += 0.015625 * Noise(uv + sin(t));
	return fbm / 0.96875;
}

float WindNoise(float2 uv)
{
	return FBM(uv + FBM(uv + FBM(uv)));
}

[numthreads(8, 8, 1)]
void CS(uint3 threadID : SV_DispatchThreadID)
{
	const uint2 pixelCoord = threadID.xy;
	const float2 textureUV = pixelCoord / TextureSize;

	OutputTexture[pixelCoord] = WindNoise(2.0f * textureUV);
}