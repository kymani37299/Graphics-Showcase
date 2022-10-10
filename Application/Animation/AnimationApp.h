#pragma once

#include <Engine/Common.h>
#include <Engine/Core/Application.h>
#include <Engine/System/ApplicationConfiguration.h>
#include <Engine/Loading/ModelLoading.h>

#include "Common/Camera.h"

struct Texture;
struct Shader;
struct Buffer;

namespace AnimationAppGUI
{
	class MorphsGUI;
}

class AnimationApp : public Application
{
	friend class AnimationAppGUI::MorphsGUI;
public:
	void OnInit(GraphicsContext& context) override;
	void OnDestroy(GraphicsContext& context) override;

	Texture* OnDraw(GraphicsContext& context) override;
	void OnUpdate(GraphicsContext& context, float dt) override;

	void OnShaderReload(GraphicsContext& context) override;
	void OnWindowResize(GraphicsContext& context) override;

private:
	Camera m_Camera = Camera::CreatePerspective(75.0f, (float)AppConfig.WindowWidth / AppConfig.WindowHeight, 0.1f, 1000.0f);

	ScopedRef<Buffer> m_EmptyBuffer;

	ScopedRef<Texture> m_FinalResult;
	ScopedRef<Texture> m_DepthTexture;

	ScopedRef<Shader> m_GeometryShader;
	ScopedRef<Shader> m_BackgroundShader;

	bool m_EnableWeightAnimation = true;
	float m_AnimationTime = 0.0f;
	std::vector<ModelLoading::SceneObject> m_SceneObjects;
};