#include "GrassApp.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Texture.h>
#include <Engine/Render/Shader.h>
#include <Engine/Loading/ModelLoading.h>
#include <Engine/Loading/TextureLoading.h>
#include <Engine/Utility/Random.h>
#include <Engine/Utility/MathUtility.h>
#include <Engine/System/Input.h>

#include "Common/DebugRender.h"
#include "Common/ConstantBuffer.h"
#include "Grass/GrassAppGUI.h"
#include "Grass/Settings.h"

static const Float3 SkyColor = Float3(135.0f, 206.0f, 235.0f) / 255.0f;
GrassSettingsCB GrassSettings;
GrassGenerationConfiguration GrassGenConfig;
GrassStatistics GrassStats;

BoundingSphere GetPatchInstanceSphere(uint32_t x, uint32_t y)
{
	const float patchStep = 1.0f / GrassPatchSubdivision;
	const Float2 patchScale = Float2{ patchStep , patchStep };
	const Float2 patchOffset = Float2{ (float)x, (float)y } * patchScale;
	const Float2 localPosition = patchOffset - Float2{0.5f, 0.5f} + patchScale / 2.0f;

	BoundingSphere sphere{};
	sphere.Center = GrassGenConfig.PlanePosition + Float3{ localPosition.x, 0.0f, localPosition.y } * GrassGenConfig.PlaneScale;
	sphere.Radius = max(patchStep * GrassGenConfig.PlaneScale.x, max(GrassGenConfig.PlaneScale.y, patchStep * GrassGenConfig.PlaneScale.z));
	return sphere;
}

struct Buffer* GenerateGrassPatchInstanceData(GraphicsContext& context, uint32_t x, uint32_t y)
{
	struct GrassInstance
	{
		DirectX::XMFLOAT2 Position;
		DirectX::XMFLOAT3 Normal;
		float Height;
	};

	const float patchStep = 1.0f / GrassPatchSubdivision;
	const Float2 patchScale = Float2{ patchStep , patchStep };
	const Float2 patchOffset = Float2{ (float)x, (float)y } * patchScale;

	const uint32_t numPatchInstances = GrassGenConfig.NumInstances / (GrassPatchSubdivision * GrassPatchSubdivision);
	std::vector<GrassInstance> instanceData{};
	instanceData.resize(numPatchInstances);
	for (uint32_t i = 0; i < numPatchInstances; i++)
	{
		Float2 position = Float2{ Random::UNorm(), Random::UNorm() };
		position *= patchScale;
		position += patchOffset;
		position -= Float2{ 0.5f, 0.5f };

		instanceData[i].Position = position.ToXMFA();
		instanceData[i].Normal = Float3(Random::SNorm(), 0.0f, Random::SNorm()).Normalize().ToXMF();
		instanceData[i].Height = Random::Float(GrassGenConfig.HeightRange.x, GrassGenConfig.HeightRange.y);
	}

	ResourceInitData initData{ &context, instanceData.data() };
	return GFX::CreateBuffer((uint32_t)instanceData.size() * sizeof(GrassInstance), sizeof(GrassInstance), RCF_None, &initData);
}

static Buffer* GenerateGrassPlane(GraphicsContext& context)
{
	static const uint32_t GrassPlaneSubdivision = 100;

	using PlaneVertex = DirectX::XMFLOAT2;

	const float subdivisionStep = 1.0f / GrassPlaneSubdivision;
	const auto getVertex = [&subdivisionStep](uint32_t x, uint32_t y)
	{
		return (subdivisionStep * Float2{ (float)x, (float)y } - Float2{ 0.5f, 0.5f }).ToXMF();
	};
	
	std::vector<PlaneVertex> vertData{};
	for (uint32_t i = 0; i < GrassPlaneSubdivision - 1; i++)
	{
		for (uint32_t j = 0; j < GrassPlaneSubdivision - 1; j++)
		{
			vertData.push_back(getVertex(i, j));
			vertData.push_back(getVertex(i, j + 1));
			vertData.push_back(getVertex(i + 1, j + 1));

			vertData.push_back(getVertex(i, j));
			vertData.push_back(getVertex(i + 1, j + 1));
			vertData.push_back(getVertex(i + 1, j));
		}
	}

	ResourceInitData initData{};
	initData.Context = &context;
	initData.Data = vertData.data();
	return GFX::CreateBuffer((uint32_t)vertData.size() * sizeof(PlaneVertex), sizeof(PlaneVertex), RCF_None, &initData);
}

void GrassApp::OnInit(GraphicsContext& context)
{
	// Load grass objects
	ModelLoading::Loader loader{ context };
	auto scene = loader.Load("Application/Grass/Resources/grass_lowpoly.gltf");
	ASSERT_CORE(!scene.empty(), "Failed to load a grass object!");
	m_GrassObject_LowPoly = scene[0];
	for (uint32_t i = 1; i < scene.size(); i++) ModelLoading::Free(scene[i]);
	scene = loader.Load("Application/Grass/Resources/grass_highpoly.gltf");
	ASSERT_CORE(!scene.empty(), "Failed to load a grass object!");
	m_GrassObject_HighPoly = scene[0];
	for (uint32_t i = 1; i < scene.size(); i++) ModelLoading::Free(scene[i]);

	m_BackgroundShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/background.hlsl"));

	m_HeightMap = ScopedRef<Texture>(TextureLoading::LoadTexture(context, "Application/Grass/Resources/HeightMap.jpg", RCF_None));
	m_GrassPlaneVB = ScopedRef<Buffer>(GenerateGrassPlane(context));
	m_GrassPlaneShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/grass_plane.hlsl"));

	m_WindTexture = ScopedRef<Texture>(GFX::CreateTexture(512, 512, RCF_Bind_UAV));
	m_WindShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/wind_texture.hlsl"));

	m_GrassShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/grass.hlsl"));

	m_Camera.Position = Float3{ -7.6f, 18.3f, -15.0f };
	m_Camera.Rotation = Float3(-0.8f, 183.0f, 0.0f);

	GrassAppGUI::AddGUI(this);
	RegenerateGrass(context);
	OnWindowResize(context);
}

void GrassApp::OnDestroy(GraphicsContext& context)
{
	ModelLoading::Free(m_GrassObject_LowPoly);
	ModelLoading::Free(m_GrassObject_HighPoly);
	GrassAppGUI::RemoveGUI();
}

Texture* GrassApp::OnDraw(GraphicsContext& context)
{
	struct PlaneParamsCB
	{
		DirectX::XMFLOAT3A Position;
		DirectX::XMFLOAT3A Scale;
		DirectX::XMFLOAT3A Color;
	};
	PlaneParamsCB planeParams{};
	planeParams.Position = GrassGenConfig.PlanePosition.ToXMFA();
	planeParams.Scale = GrassGenConfig.PlaneScale.ToXMFA();
	planeParams.Color = GrassSettings.BottomColor;

	// Clear targets
	GFX::Cmd::ClearRenderTarget(context, m_FinalResult.get());
	GFX::Cmd::ClearDepthStencil(context, m_DepthTexture.get());

	// Background
	{
		GFX::Cmd::MarkerBegin(context, "Background");

		ConstantBuffer cb{};
		cb.Add(SkyColor);

		GraphicsState state{};
		state.Table.CBVs.push_back(cb.GetBuffer());
		GFX::Cmd::BindShader(state, m_BackgroundShader.get(), VS | PS);
		GFX::Cmd::BindRenderTarget(state, m_FinalResult.get());
		GFX::Cmd::DrawFC(context, state);
		GFX::Cmd::BindState(context, state);
		GFX::Cmd::MarkerEnd(context);
	}

	// Draw plane
	{
		GFX::Cmd::MarkerBegin(context, "Grass plane");

		ConstantBuffer cb{};
		cb.Add(m_Camera.ConstantData);
		cb.Add(planeParams);

		GraphicsState state{};
		state.Pipeline.DepthStencilState.DepthEnable = true;
		state.Table.SRVs.push_back(m_HeightMap.get());
		state.Table.CBVs.push_back(cb.GetBuffer());
		state.VertexBuffers.push_back(m_GrassPlaneVB.get());
		GFX::Cmd::BindSampler(state, 0, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		GFX::Cmd::BindShader(state, m_GrassPlaneShader.get(), VS | PS);
		GFX::Cmd::BindRenderTarget(state, m_FinalResult.get());
		GFX::Cmd::BindDepthStencil(state, m_DepthTexture.get());
		GFX::Cmd::BindState(context, state);
		context.CmdList->DrawInstanced(m_GrassPlaneVB->ByteSize / m_GrassPlaneVB->Stride, 1, 0, 0);

		GFX::Cmd::MarkerEnd(context);
	}

	// Generate wind
	{
		GFX::Cmd::MarkerBegin(context, "Generate wind texture");

		ConstantBuffer cb{};
		cb.Add((float)m_WindTexture->Width);
		cb.Add((float)m_WindTexture->Height);
		cb.Add(m_TimeSeconds);

		GraphicsState state{};
		state.Table.UAVs.push_back(m_WindTexture.get());
		state.Table.CBVs.push_back(cb.GetBuffer());
		GFX::Cmd::BindShader(state, m_WindShader.get(), CS);
		GFX::Cmd::BindState(context, state);
		context.CmdList->Dispatch(MathUtility::CeilDiv(m_WindTexture->Width, 8), MathUtility::CeilDiv(m_WindTexture->Width, 8), 1);

		GFX::Cmd::MarkerEnd(context);
	}

	// Draw grass
	{
		GFX::Cmd::MarkerBegin(context, "Grass");

		ConstantBuffer cb{};
		cb.Add(m_Camera.ConstantData);
		cb.Add(XMUtility::ToHLSLFloat4x4(m_GrassObject_HighPoly.ModelToWorld)); // LowPoly should have same model to world (if not put this in push constants)
		cb.Add(GrassSettings);
		cb.Add(planeParams);
		cb.Add(SkyColor.ToXMF());

		GraphicsState state{};
		state.Pipeline.DepthStencilState.DepthEnable = true;
		state.Table.CBVs.push_back(cb.GetBuffer());
		state.Table.SRVs.push_back(nullptr);
		state.Table.SRVs.push_back(m_WindTexture.get());
		state.Table.SRVs.push_back(m_HeightMap.get());
		state.VertexBuffers.resize(1);

		GFX::Cmd::BindSampler(state, 0, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		GFX::Cmd::BindShader(state, m_GrassShader.get(), VS | PS);
		GFX::Cmd::BindRenderTarget(state, m_FinalResult.get());
		GFX::Cmd::BindDepthStencil(state, m_DepthTexture.get());

		GrassStats.PatchesDrawn = 0;
		for (uint32_t i = 0; i < GrassPatchSubdivision; i++)
		{
			for (uint32_t j = 0; j < GrassPatchSubdivision; j++)
			{
				constexpr float lowpolyTreshold = 10.0f;
				constexpr float lowpolyHeightTreshold = 10.0f;
				const BoundingSphere bs = GetPatchInstanceSphere(i, j);
				const float patchDistance = (bs.Center - m_Camera.Position).Length() - bs.Radius;
				const float patchHeightDistance = m_Camera.Position.y - GrassGenConfig.PlanePosition.y - GrassGenConfig.PlaneScale.y;
				if (patchDistance > lowpolyTreshold || patchHeightDistance > lowpolyHeightTreshold)
				{
					state.VertexBuffers[0] = m_GrassObject_LowPoly.Vertices;
					state.IndexBuffer = m_GrassObject_LowPoly.Indices;
				}
				else
				{
					state.VertexBuffers[0] = m_GrassObject_HighPoly.Vertices;
					state.IndexBuffer = m_GrassObject_HighPoly.Indices;
				}

				if (m_Camera.CameraFrustum.IsInFrustum(bs))
				{
					GrassStats.PatchesDrawn++;
					const uint32_t PatchNumInstances = GrassGenConfig.NumInstances / (GrassPatchSubdivision * GrassPatchSubdivision);
					state.Table.SRVs[0] = m_GrassPatches[i][j].get();
					GFX::Cmd::BindState(context, state);
					context.CmdList->DrawIndexedInstanced(state.IndexBuffer->ByteSize / state.IndexBuffer->Stride, PatchNumInstances, 0, 0, 0);
				}
			}
		}
		GrassStats.InstancesDrawn = GrassStats.PatchesDrawn * (GrassGenConfig.NumInstances / (GrassPatchSubdivision * GrassPatchSubdivision));

		GFX::Cmd::MarkerEnd(context);
	}

	return m_FinalResult.get();
}

void GrassApp::OnUpdate(GraphicsContext& context, float dt)
{
	m_Camera.Update(dt);
	m_TimeSeconds += dt / 1000.0f;

	if (Input::IsKeyJustPressed('C'))
	{
		m_Camera.FreezeFrustum = !m_Camera.FreezeFrustum;
	}
}

void GrassApp::OnShaderReload(GraphicsContext& context)
{

}

void GrassApp::OnWindowResize(GraphicsContext& context)
{
	m_FinalResult = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF_Bind_RTV));
	m_DepthTexture = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF_Bind_DSV));
	m_Camera.AspectRatio = (float)AppConfig.WindowWidth / AppConfig.WindowHeight;
}

void GrassApp::RegenerateGrass(GraphicsContext& context)
{
	GrassStats.TotalInstances = GrassGenConfig.NumInstances;
	GrassStats.TotalPatches = GrassPatchSubdivision * GrassPatchSubdivision;

	for (uint32_t i = 0; i < GrassPatchSubdivision; i++)
	{
		for (uint32_t j = 0; j < GrassPatchSubdivision; j++)
		{
			m_GrassPatches[i][j] = ScopedRef<Buffer>(GenerateGrassPatchInstanceData(context, i, j));
		}
	}
}
