#include "DebugRender.h"

#include <Engine/Render/Context.h>
#include <Engine/Render/Commands.h>
#include <Engine/Render/Shader.h>
#include <Engine/Render/Buffer.h>

#include "Common/Camera.h"
#include "Common/ConstantBuffer.h"

struct GraphicsContext;

namespace DebugRender
{
	static Buffer* SphereVB = nullptr;
	static Shader* DebugShader = nullptr;
	static GraphicsState DebugState{};

	static Buffer* GenerateSphereVB(GraphicsContext& context)
	{
		constexpr uint32_t parallels = 11;
		constexpr uint32_t meridians = 22;
		constexpr float PI = 3.14159265358979323846f;

		std::vector<Float3> verticesRaw;
		std::vector<Float3> vertices;

		verticesRaw.push_back({ 0.0f, 1.0f, 0.0f });
		for (uint32_t j = 0; j < parallels - 1; ++j)
		{
			float polar = PI * float(j + 1) / float(parallels);
			float sp = std::sin(polar);
			float cp = std::cos(polar);
			for (uint32_t i = 0; i < meridians; ++i)
			{
				float azimuth = 2.0f * PI * float(i) / float(meridians);
				float sa = std::sin(azimuth);
				float ca = std::cos(azimuth);
				float x = sp * ca;
				float y = cp;
				float z = sp * sa;
				verticesRaw.push_back({ x, y, z });
			}
		}
		verticesRaw.push_back({ 0.0f, -1.0f, 0.0f });

		for (uint32_t i = 0; i < meridians; ++i)
		{
			uint32_t const a = i + 1;
			uint32_t const b = (i + 1) % meridians + 1;
			vertices.push_back(verticesRaw[0]);
			vertices.push_back(verticesRaw[b]);
			vertices.push_back(verticesRaw[a]);
		}

		for (uint32_t j = 0; j < parallels - 2; ++j)
		{
			uint32_t aStart = j * meridians + 1;
			uint32_t bStart = (j + 1) * meridians + 1;
			for (uint32_t i = 0; i < meridians; ++i)
			{
				const uint32_t a = aStart + i;
				const uint32_t a1 = aStart + (i + 1) % meridians;
				const uint32_t b = bStart + i;
				const uint32_t b1 = bStart + (i + 1) % meridians;
				vertices.push_back(verticesRaw[a]);
				vertices.push_back(verticesRaw[a1]);
				vertices.push_back(verticesRaw[b1]);
				vertices.push_back(verticesRaw[b]);
				vertices.push_back(verticesRaw[a]);
				vertices.push_back(verticesRaw[b1]);
			}
		}

		for (uint32_t i = 0; i < meridians; ++i)
		{
			uint32_t const a = i + meridians * (parallels - 2) + 1;
			uint32_t const b = (i + 1) % meridians + meridians * (parallels - 2) + 1;
			vertices.push_back(verticesRaw[verticesRaw.size() - 1]);
			vertices.push_back(verticesRaw[a]);
			vertices.push_back(verticesRaw[b]);
		}

		ResourceInitData initData{ &context , vertices.data() };
		return GFX::CreateVertexBuffer<Float3>((uint32_t) vertices.size(), &initData);
	}

	void Init(GraphicsContext& context)
	{
		SphereVB = GenerateSphereVB(context);
		DebugShader = new Shader{ "Application/Common/debug_render.hlsl" };

		DebugState.Shader = DebugShader;
		DebugState.VertexBuffers.push_back(SphereVB);
		DebugState.Table.CBVs.resize(1);
		DebugState.DepthStencilState.DepthEnable = true;
		DebugState.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		DebugState.BlendState.RenderTarget[0].BlendEnable = true;
		DebugState.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		DebugState.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
		DebugState.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		DebugState.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		DebugState.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		DebugState.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	}

	void Deinit()
	{
		delete SphereVB;
	}

	void DrawSphere(GraphicsContext& context, Camera& camera, Texture* colorTarget, Texture* depthDarget, Float3 position, float radius, Float4 color)
	{
		ConstantBuffer cb{};
		cb.Add(camera.ConstantData);
		cb.Add(color);
		cb.Add(position);
		cb.Add(radius);

		DebugState.Table.CBVs[0] = cb.GetBuffer();
		DebugState.RenderTargets.push_back(colorTarget);
		DebugState.DepthStencil = depthDarget;
		GFX::Cmd::BindState(context, DebugState);
		context.CmdList->DrawInstanced(SphereVB->ByteSize / SphereVB->Stride, 1, 0, 0);
	}
}