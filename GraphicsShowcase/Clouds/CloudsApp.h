#pragma once

#include <Engine/Common.h>
#include <Engine/Core/Application.h>
#include <Engine/System/ApplicationConfiguration.h>

#include "Common/Camera.h"

struct Texture;
struct Shader;

class CloudsApp : public Application
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

	ScopedRef<Shader> m_CloudsShader;

	ScopedRef<Texture> m_CloudNoise;
	ScopedRef<Texture> m_CloudDetailNoise;

	ScopedRef<Texture> m_FinalResult;
};