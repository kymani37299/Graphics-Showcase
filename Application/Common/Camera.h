#pragma once

#include <Engine/Common.h>

struct Camera;

struct BoundingSphere
{
	Float3 Center{ 0.0f, 0.0f, 0.0f };
	float Radius{ 1.0f };
};

struct ViewFrustum
{
	void Update(const Camera& camera);
	bool IsInFrustum(const BoundingSphere& sphere);

	Float4 Planes[6];
};

struct Camera
{
	static float s_CameraSpeed;

	struct CameraConstantData
	{
		DirectX::XMFLOAT4X4 WorldToView;
		DirectX::XMFLOAT4X4 ViewToClip;
		DirectX::XMFLOAT4X4 ClipToWorld;

		DirectX::XMFLOAT3A Position;
		DirectX::XMFLOAT3A Forward;
		DirectX::XMFLOAT3A Right;
		DirectX::XMFLOAT3A Up;
	};

	static Camera CreatePerspective(float fov, float aspect, float znear, float zfar);
	static Camera CreateOrtho(float rectWidth, float rectHeight, float znear, float zfar);

	void Update(float dt);
	void UpdateConstantData();

	enum class CameraType
	{
		Ortho,
		Perspective,
	};

	Float3 Position{ 0.0f, 0.0f, 0.0f };

	bool UseRotation = true; // Uses rotation for updating forward vector
	Float3 Rotation{ 0.0f, 0.0f, 0.0f }; // (Pitch, Yaw, Roll)

	Float3 Forward{ 1.0f, 0.0f, 0.0f };
	Float3 Up{ 0.0f, 1.0f, 0.0f };
	Float3 Right{ 0.0f, 0.0f, 1.0f };

	CameraType Type;
	float ZFar;
	float ZNear;

	// Perspective only
	float AspectRatio;
	float FOV;

	// Ortho
	float RectWidth;
	float RectHeight;

	bool FreezeFrustum = false;
	ViewFrustum CameraFrustum;
	CameraConstantData ConstantData;
};