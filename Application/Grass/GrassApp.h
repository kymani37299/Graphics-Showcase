#pragma once

#include <Engine/Common.h>
#include <Engine/Core/Application.h>
#include <Engine/Loading/ModelLoading.h>
#include <Engine/System/ApplicationConfiguration.h>

#include "Common/Camera.h"

struct Texture;
struct Shader;
struct Buffer;

static constexpr uint32_t GrassPatchSubdivision = 12;

class GrassApp : public Application
{
public:
	void OnInit(GraphicsContext& context) override;
	void OnDestroy(GraphicsContext& context) override;

	Texture* OnDraw(GraphicsContext& context) override;
	void OnUpdate(GraphicsContext& context, float dt) override;

	void OnShaderReload(GraphicsContext& context) override;
	void OnWindowResize(GraphicsContext& context) override;

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

	ScopedRef<Buffer> m_GrassPatches[GrassPatchSubdivision][GrassPatchSubdivision];
	ScopedRef<Shader> m_GrassShader;

	ModelLoading::SceneObject m_GrassObject_LowPoly;
	ModelLoading::SceneObject m_GrassObject_HighPoly;
	Camera m_Camera = Camera::CreatePerspective(75.0f, (float)AppConfig.WindowWidth / AppConfig.WindowHeight, 0.1f, 100.0f);
	float m_TimeSeconds = 0.0f;
};

