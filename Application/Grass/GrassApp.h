#pragma once

#include <deque>

#include <Engine/Common.h>
#include <Engine/Core/Application.h>
#include <Engine/Loading/ModelLoading.h>
#include <Engine/System/ApplicationConfiguration.h>

#include "Common/Camera.h"

struct Texture;
struct Shader;
struct Buffer;

static constexpr uint32_t GrassPatchSubdivision = 32;

struct GrassPatchData
{
	uint32_t InstanceDataOffset = 0;
};

struct GrassMaterialData
{
	float Probabilty = 1.0f;
	ModelLoading::SceneObject LowPoly;
	ModelLoading::SceneObject HighPoly;
};

class GrassApp : public Application
{
public:
	void OnInit(GraphicsContext& context) override;
	void OnDestroy(GraphicsContext& context) override;

	Texture* OnDraw(GraphicsContext& context) override;
	void OnUpdate(GraphicsContext& context, float dt) override;

	void OnShaderReload(GraphicsContext& context) override;
	void OnWindowResize(GraphicsContext& context) override;

private:
	void RegenerateGrass(GraphicsContext& context);

private:
	ScopedRef<Texture> m_FinalResult;
	ScopedRef<Texture> m_DepthTexture;

	ScopedRef<Shader> m_BackgroundShader;

	ScopedRef<Texture> m_HeightMap;
	ScopedRef<Buffer> m_GrassPlaneVB;
	ScopedRef<Shader> m_GrassPlaneShader;

	ScopedRef<Texture> m_WindTexture;
	ScopedRef<Shader> m_WindShader;

	ScopedRef<Buffer> m_GrassInstanceData;
	ScopedRef<Buffer> m_GrassPatchDataBuffer;
	ScopedRef<Shader> m_GrassPrepareShader;
	ScopedRef<Shader> m_GrassShader;

	ScopedRef<Buffer> m_IndirectArgsBufferHP;
	ScopedRef<Buffer> m_IndirectArgsCountBufferHP;

	ScopedRef<Buffer> m_IndirectArgsBufferLP;
	ScopedRef<Buffer> m_IndirectArgsCountBufferLP;

	std::vector<GrassMaterialData> m_GrassMaterials;

	Camera m_Camera = Camera::CreatePerspective(75.0f, (float)AppConfig.WindowWidth / AppConfig.WindowHeight, 0.1f, 500.0f);
	float m_TimeSeconds = 0.0f;

	bool m_ShowWindTexture = false;
};

