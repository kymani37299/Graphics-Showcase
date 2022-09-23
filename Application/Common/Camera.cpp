#include "Camera.h"

#include <algorithm>

#include <Engine/Common.h>
#include <Engine/System/Input.h>
#include <Engine/System/Window.h>

static Float3 CalcForwardVector(float pitch, float yaw)
{
	const float x = std::cosf(yaw) * std::cosf(pitch);
	const float y = std::sinf(pitch);
	const float z = std::sinf(yaw) * std::cos(pitch);
	return Float3{ x,y,z }.Normalize();
}

static float DegreesToRadians(float deg)
{
	const float DEG_2_RAD = 3.1415f / 180.0f;
	return deg * DEG_2_RAD;
}

// p0,p1,p2 must be in ccw order
static Float4 CreateFrustumPlane(Float3 p0, Float3 p1, Float3 p2)
{
	const Float3 u = p1 - p0;
	const Float3 v = p2 - p0;
	const Float3 n = v.Cross(u).Normalize();

	Float4 plane;
	plane.x = n.x;
	plane.y = n.y;
	plane.z = n.z;
	plane.w = -n.x * p0.x - n.y * p0.y - n.z * p0.z;

	return plane;
}

void ViewFrustum::Update(const Camera& c)
{
	if (c.Type == Camera::CameraType::Perspective)
	{
		// Near and far plane dimensions
		const float fovTan = tanf(DegreesToRadians(c.FOV / 2.0f));
		const float hnear = 2.0f * fovTan * c.ZNear;
		const float wnear = hnear * c.AspectRatio;
		const float hfar = 2.0f * fovTan * c.ZFar;
		const float wfar = hfar * c.AspectRatio;

		// 8 points of the frustum
		const Float3 fc = c.Position + c.ZFar * c.Forward;
		const Float3 nc = c.Position + c.ZNear * c.Forward;

		const Float3 ftl = fc + (hfar / 2.0f) * c.Up - (wfar / 2.0f) * c.Right;
		const Float3 ftr = fc + (hfar / 2.0f) * c.Up + (wfar / 2.0f) * c.Right;
		const Float3 fbl = fc - (hfar / 2.0f) * c.Up - (wfar / 2.0f) * c.Right;
		const Float3 fbr = fc - (hfar / 2.0f) * c.Up + (wfar / 2.0f) * c.Right;

		const Float3 ntl = nc + (hnear / 2.0f) * c.Up - (wnear / 2.0f) * c.Right;
		const Float3 ntr = nc + (hnear / 2.0f) * c.Up + (wnear / 2.0f) * c.Right;
		const Float3 nbl = nc - (hnear / 2.0f) * c.Up - (wnear / 2.0f) * c.Right;
		const Float3 nbr = nc - (hnear / 2.0f) * c.Up + (wnear / 2.0f) * c.Right;

		Planes[0] = CreateFrustumPlane(ntl, ntr, ftl);	// Top
		Planes[1] = CreateFrustumPlane(nbr, nbl, fbr);	// Bottom
		Planes[2] = CreateFrustumPlane(nbl, ntl, fbl);	// Left
		Planes[3] = CreateFrustumPlane(ntr, nbr, fbr);	// Right
		Planes[4] = CreateFrustumPlane(ntr, ntl, nbr);	// Near
		Planes[5] = CreateFrustumPlane(ftl, ftr, fbl);	// Far
	}
	else
	{
		NOT_IMPLEMENTED;
	}
}

bool ViewFrustum::IsInFrustum(const BoundingSphere& sphere)
{
	for (uint32_t i = 0; i < 6; i++)
	{
		const float signedDistance = Float4{ sphere.Center.x, sphere.Center.y, sphere.Center.z, 1.0f }.Dot(Planes[i]);
		if (signedDistance < -sphere.Radius) return false;
	}
	return true;
}

Camera Camera::CreatePerspective(float fov, float aspect, float znear, float zfar)
{
	Camera cam;
	cam.Type = CameraType::Perspective;
	cam.AspectRatio = aspect;
	cam.FOV = fov;
	cam.ZNear = znear;
	cam.ZFar = zfar;
	return cam;
}

Camera Camera::CreateOrtho(float rectWidth, float rectHeight, float znear, float zfar)
{
	Camera cam;
	cam.Type = CameraType::Ortho;
	cam.RectWidth = rectWidth;
	cam.RectHeight = rectHeight;
	cam.ZNear = znear;
	cam.ZFar;
	return cam;
}

void Camera::Update(float dt)
{
	// Update Input
	{
		float dtSec = dt / 1000.0f;

		const float movement_speed = 10.0f;
		const float mouse_speed = 1000.0f;
		char mov_inputs[] = { 'W', 'S', 'A', 'D', 'Q', 'E' };
		Float3 mov_effects[] = { {0.0f, 0.0f, 1.0f},{0.0f, 0.0f, -1.0f},{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},{0.0f, -1.0f, 0.0f},{0.0f, 1.0f, 0.0f} };
		static_assert(STATIC_ARRAY_SIZE(mov_inputs) == STATIC_ARRAY_SIZE(mov_effects));

		float movement_factor = 1.0f;
		if (Input::IsKeyPressed(VK_SHIFT))
		{
			movement_factor = 10.0f;
		}

		Float3 moveDir{ 0.0f, 0.0f, 0.0f };
		for (uint16_t i = 0; i < STATIC_ARRAY_SIZE(mov_inputs); i++)
		{
			if (Input::IsKeyPressed(mov_inputs[i]))
				moveDir += dtSec * movement_factor * movement_speed * mov_effects[i];
		}

		const DirectX::XMFLOAT4X4 worldtoView{ Right.x, Right.y, Right.z, 0.0f,
											   Up.x, Up.y, Up.z, 0.0f,
											   Forward.x, Forward.y, Forward.z, 0.0f,
										       0.0f, 0.0f, 0.0f, 1.0f };
		const DirectX::XMMATRIX cameraWorldToView = DirectX::XMLoadFloat4x4(&worldtoView);
		Float4 moveDir4{ moveDir.x, moveDir.y, moveDir.z, 1.0f };
		Float4 relativeDir = Float4(DirectX::XMVector4Transform(moveDir4.ToXM(), cameraWorldToView));
		Position += Float3{ relativeDir.x, relativeDir.y , relativeDir.z };

		if (!Window::Get()->IsCursorShown())
		{
			Float2 mouseDelta = Input::GetMouseDelta();
			Rotation.y -= dtSec * mouse_speed * mouseDelta.x;
			Rotation.x -= dtSec * mouse_speed * mouseDelta.y;
			Rotation.x = std::clamp(Rotation.x, -1.5f, 1.5f);
		}

		char camera_inputs[] = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT };
		Float2 camera_effects[] = { {1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f} };
		static_assert(STATIC_ARRAY_SIZE(camera_inputs) == STATIC_ARRAY_SIZE(camera_effects));

		Float2 cameraDir{ 0.0f, 0.0f };
		for (uint16_t i = 0; i < STATIC_ARRAY_SIZE(camera_inputs); i++)
		{
			if (Input::IsKeyPressed(camera_inputs[i]))
				cameraDir += dtSec * mouse_speed * camera_effects[i];
		}

		Rotation.y += dtSec * mouse_speed * 0.0001f * cameraDir.y;
		Rotation.x += dtSec * mouse_speed * 0.0001f * cameraDir.x;
		Rotation.x = std::clamp(Rotation.x, -1.5f, 1.5f);
	}

	// Update transforms
	{
		using namespace DirectX;

		if (UseRotation)
		{
			Forward = CalcForwardVector(Rotation.x, Rotation.y);
		}

		// TODO: Calc up based on roll
		Up = Float3(0.0f, 1.0f, 0.0f);
		Right = Forward.Cross(Up).Normalize();
		Up = Right.Cross(Forward).Normalize();

		XMMATRIX worldToView = XMMatrixLookAtLH(Position, (Position + Forward), Up);
		XMMATRIX viewToClip;

		if (Type == CameraType::Perspective) viewToClip = XMMatrixPerspectiveFovLH(DegreesToRadians(FOV), AspectRatio, ZNear, ZFar);
		else if (Type == CameraType::Ortho) viewToClip = XMMatrixOrthographicLH(RectWidth, RectHeight, ZNear, ZFar);

		XMMATRIX worldToClip = XMMatrixMultiply(worldToView, viewToClip);
		XMMATRIX clipToWorld = XMMatrixInverse(nullptr, worldToClip);

		ConstantData.WorldToView = XMUtility::ToHLSLFloat4x4(worldToView);
		ConstantData.ViewToClip = XMUtility::ToHLSLFloat4x4(viewToClip);
		ConstantData.ClipToWorld = XMUtility::ToHLSLFloat4x4(clipToWorld);
		ConstantData.Position = Position.ToXMFA();
		ConstantData.Forward = Forward.ToXMFA();
		ConstantData.Right = Right.ToXMFA();
		ConstantData.Up = Up.ToXMFA();
	}

	if (!FreezeFrustum)
	{
		CameraFrustum.Update(*this);
	}
}