#pragma once

#include <Engine/Common.h>
#include <Engine/Core/Application.h>
#include <Engine/System/ApplicationConfiguration.h>

#include "Common/Camera.h"
#include "Loading/ModelLoading.h"

struct Texture;
struct Shader;

class PBRApp : public Application
{
public:
	void OnInit(GraphicsContext& context) override;
	void OnDestroy(GraphicsContext& context) override;

	Texture* OnDraw(GraphicsContext& context) override;
	void OnUpdate(GraphicsContext& context, float dt) override;

	void OnShaderReload(GraphicsContext& context) override;
	void OnWindowResize(GraphicsContext& context) override;

private:
	Camera m_Camera = Camera::CreatePerspective(75.0f, (float) AppConfig.WindowWidth/AppConfig.WindowHeight, 0.1f, 1000.0f);

	ScopedRef<Shader> m_PBRShader;
	ScopedRef<Shader> m_BackgroundShader;

	ScopedRef<Texture> m_FinalResult;
	ScopedRef<Texture> m_DepthTexture;

	ModelLoading::Scene m_Scene;
	float m_TimeSinceStarted = 0.0f;
};