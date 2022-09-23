#pragma once

#include <Engine/Common.h>

struct Texture;
struct GraphicsContext;
struct Camera;

namespace DebugRender
{
	void Init(GraphicsContext& context);
	void Deinit();

	void DrawSphere(GraphicsContext& context, Camera& camera, Texture* colorTarget, Texture* depthDarget, Float3 position, float radius = 1.0f, Float4 color = { 1.0f, 0.0f, 0.0f, 0.5f });
}