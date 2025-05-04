#pragma once

#include <vector>

#include <Engine/Common.h>
#include <Engine/Core/Application.h>
#include <Engine/Loading/ModelLoading.h>
#include <Engine/System/ApplicationConfiguration.h>

#include "Common/Camera.h"

struct Texture;
struct Shader;
struct Buffer;

class MeshletsApp : public Application
{
public:
	void OnInit(GraphicsContext& context) override;
	void OnDestroy(GraphicsContext& context) override;

	Texture* OnDraw(GraphicsContext& context) override;
	void OnUpdate(GraphicsContext& context, float dt) override;

	void OnShaderReload(GraphicsContext& context) override;
	void OnWindowResize(GraphicsContext& context) override;

private:
	ScopedRef<Texture> m_FinalResult;
	ScopedRef<Texture> m_DepthTexture;
	ScopedRef<Shader> m_Shader;

	Camera m_Camera = Camera::CreatePerspective(75.0f, (float)AppConfig.WindowWidth / AppConfig.WindowHeight, 0.1f, 500.0f);

	static constexpr uint32_t NUM_INSTANCES = 1000;
	ModelLoading::Scene m_Scene;

	ScopedRef<Buffer> m_MeshletsBuffer;
	ScopedRef<Buffer> m_UniqueVertexIB;
	ScopedRef<Buffer> m_MeshletsTriangleBuffer;
};

