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