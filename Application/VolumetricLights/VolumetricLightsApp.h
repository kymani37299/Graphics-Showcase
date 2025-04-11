#pragma once

#include <vector>

#include <Engine/Common.h>
#include <Engine/Core/Application.h>
#include <Engine/System/ApplicationConfiguration.h>

#include "Common/Camera.h"
#include "Loading/ModelLoading.h"

struct Texture;
struct Shader;

class VolumetricLightsApp : public Application
{
public:
	void OnInit(GraphicsContext& context) override;
	void OnDestroy(GraphicsContext& context) override;

	Texture* OnDraw(GraphicsContext& context) override;
	void OnUpdate(GraphicsContext& context, float dt) override;

	void OnShaderReload(GraphicsContext& context) override;
	void OnWindowResize(GraphicsContext& context) override;

private:
	Camera m_Camera = Camera::CreatePerspective(75.0f, (float)AppConfig.WindowWidth / AppConfig.WindowHeight, 0.1f, 200.0f);

	ScopedRef<Texture> m_FinalResult;
	ScopedRef<Texture> m_DepthTexture;
	ScopedRef<Texture> m_Shadowmap;

	ScopedRef<Shader> m_BackgroundShader;
	ScopedRef<Shader> m_SceneShader;
	ScopedRef<Shader> m_ShadowShader;
	ScopedRef<Shader> m_VolumetricFogShader;

	ModelLoading::Scene m_Scene;
};

