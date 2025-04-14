#define PI 3.141592653589793238462643

struct Camera
{
	float4x4 WorldToView;
	float4x4 ViewToClip;
	float4x4 ClipToWorld;

	float3 Position;	float Pad0;
	float3 Forward;		float Pad1;
	float3 Right;		float Pad2;
	float3 Up;			float Pad3;
};

float4 GetClipPosition(const float3 worldPosition, const Camera camera)
{
	return mul( mul( float4(worldPosition, 1.0f), camera.WorldToView), camera.ViewToClip);
}

float3 GetWorldPositionFromDepth(float depth, float2 screenUV, Camera camera)
{
	// [0,1]
	float3 normalizedPosition = float3(screenUV, depth);

	// Flip the y because screenUV is 0.0 in upper left and we want down left
	normalizedPosition.y = 1.0f - normalizedPosition.y;

	// [-1,1] for xy and [0,1] for z
	const float4 clipSpacePosition = float4(normalizedPosition * float3(2.0f, 2.0f, 1.0f) + float3(-1.0f, -1.0f, 0.0f), 1.0f);
	const float4 worldPosition = mul(clipSpacePosition, camera.ClipToWorld);

	return worldPosition.xyz / worldPosition.w;
}

float2 GetUVFromClipPosition(float4 clipPosition)
{
	clipPosition.xyz /= clipPosition.w;

	// [-1,1] -> [0,1]
	float2 uv = clipPosition.xy * 0.5f + 0.5f;
	uv.y = 1.0f - uv.y;
	return uv;
}