#include "GrassApp.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Texture.h>
#include <Engine/Render/Shader.h>
#include <Engine/Render/RenderThread.h>
#include <Engine/Loading/ModelLoading.h>
#include <Engine/Loading/TextureLoading.h>
#include <Engine/Utility/Random.h>
#include <Engine/Utility/MathUtility.h>
#include <Engine/Utility/Timer.h>
#include <Engine/System/Input.h>

#include "Common/DebugRender.h"
#include "Common/ConstantBuffer.h"
#include "Grass/GrassAppGUI.h"
#include "Grass/Settings.h"

static const Float3 SkyColor = Float3(135.0f, 206.0f, 235.0f) / 255.0f;
GrassSettingsCB GrassSettings;
GrassPerfSettingsCB GrassPerfSettings;
GrassGenerationConfiguration GrassGenConfig;

static constexpr uint32_t INDIRECT_ARGUMENTS_STRIDE = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) + 4 * sizeof(float);
static constexpr uint32_t MAX_INDIRECT_ARGUMENTS_COUNT = GrassPatchSubdivision * GrassPatchSubdivision;

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
	return GFX::CreateBuffer((uint32_t)vertData.size() * sizeof(PlaneVertex), sizeof(PlaneVertex), RCF::None, &initData);
}

void GrassApp::OnInit(GraphicsContext& context)
{
	GrassMaterialData regularGrass{};
	regularGrass.Probabilty = 0.9f;

	// Load grass objects
	ModelLoading::Loader loader{ context };
	const auto loadGrassModel = [&loader, &context](ModelLoading::SceneObject& sceneObject, const std::string path)
	{
		auto scene = loader.Load(path);
		ASSERT_CORE(!scene.Objects.empty(), "Failed to load a grass object!");
		sceneObject = scene.Objects[0];
		for (uint32_t i = 1; i < scene.Objects.size(); i++) ModelLoading::Free(context, scene.Objects[i]);
	};
	loadGrassModel(regularGrass.LowPoly, "Application/Grass/Resources/grass_lowpoly.gltf");
	loadGrassModel(regularGrass.HighPoly, "Application/Grass/Resources/grass_highpoly.gltf");

	m_GrassMaterials.push_back(regularGrass);

	m_BackgroundShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/background.hlsl"));

	m_HeightMap = ScopedRef<Texture>(TextureLoading::LoadTexture(context, "Application/Grass/Resources/HeightMap.jpg", RCF::None));
	m_GrassPlaneVB = ScopedRef<Buffer>(GenerateGrassPlane(context));
	m_GrassPlaneShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/grass_plane.hlsl"));

	m_WindTexture = ScopedRef<Texture>(GFX::CreateTexture(512, 512, RCF::UAV));
	m_WindShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/wind_texture.hlsl"));

	m_GrassShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/grass.hlsl"));
	m_GrassPrepareShader = ScopedRef<Shader>(new Shader("Application/Grass/Shaders/prepare_draw.hlsl"));

	m_Camera.Position = Float3{ -7.6f, 18.3f, -15.0f };
	m_Camera.Rotation = Float3(-0.8f, 183.0f, 0.0f);

	m_GrassPatchDataBuffer = ScopedRef<Buffer>(GFX::CreateBuffer(sizeof(GrassPatchData) * GrassPatchSubdivision * GrassPatchSubdivision, sizeof(GrassPatchData), RCF::None));

	m_IndirectArgsBufferHP = ScopedRef<Buffer>(GFX::CreateBuffer(INDIRECT_ARGUMENTS_STRIDE * MAX_INDIRECT_ARGUMENTS_COUNT, INDIRECT_ARGUMENTS_STRIDE, RCF::UAV));
	m_IndirectArgsCountBufferHP = ScopedRef<Buffer>(GFX::CreateBuffer(sizeof(uint32_t), sizeof(uint32_t), RCF::UAV));

	m_IndirectArgsBufferLP = ScopedRef<Buffer>(GFX::CreateBuffer(INDIRECT_ARGUMENTS_STRIDE * MAX_INDIRECT_ARGUMENTS_COUNT, INDIRECT_ARGUMENTS_STRIDE, RCF::UAV));
	m_IndirectArgsCountBufferLP = ScopedRef<Buffer>(GFX::CreateBuffer(sizeof(uint32_t), sizeof(uint32_t), RCF::UAV));

	GrassAppGUI::AddGUI();
	RegenerateGrass(context);
	OnWindowResize(context);
}

void GrassApp::OnDestroy(GraphicsContext& context)
{
	for (GrassMaterialData& mat : m_GrassMaterials)
	{
		ModelLoading::Free(context, mat.LowPoly);
		ModelLoading::Free(context, mat.HighPoly);
	}
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

	// Generate wind
	{
		GFX::Cmd::MarkerBegin(context, "Generate wind texture");

		ConstantBuffer cb{};
		cb.Add((float)m_WindTexture->Width);
		cb.Add((float)m_WindTexture->Height);
		cb.Add(m_TimeSeconds);

		GraphicsState state{};
		state.Shader = m_WindShader.get();
		state.ShaderStages = CS;
		state.Table.UAVs[0] = m_WindTexture.get();
		state.Table.CBVs[0] = cb.GetBuffer(context);
		context.ApplyState(state);
		context.CmdList->Dispatch(MathUtility::CeilDiv(m_WindTexture->Width, 8u), MathUtility::CeilDiv(m_WindTexture->Width, 8u), 1);

		GFX::Cmd::MarkerEnd(context);

		if (m_ShowWindTexture) return m_WindTexture.get();
	}

	// Clear targets
	GFX::Cmd::ClearRenderTarget(context, m_FinalResult.get());
	GFX::Cmd::ClearDepthStencil(context, m_DepthTexture.get());

	// Background
	{
		GFX::Cmd::MarkerBegin(context, "Background");

		ConstantBuffer cb{};
		cb.Add(SkyColor);

		GraphicsState state{};
		state.Shader = m_BackgroundShader.get();
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.RenderTargets[0] = m_FinalResult.get();
		GFX::Cmd::DrawFC(context, state);
		GFX::Cmd::MarkerEnd(context);
	}

	// Draw plane
	{
		GFX::Cmd::MarkerBegin(context, "Grass plane");

		ConstantBuffer cb{};
		cb.Add(m_Camera.ConstantData);
		cb.Add(planeParams);
		cb.Add(SkyColor.ToXMF());

		GraphicsState state{};
		state.Shader = m_GrassPlaneShader.get();
		state.DepthStencilState.DepthEnable = true;
		state.Table.SRVs[0] = m_HeightMap.get();
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.Table.SMPs[0] = Sampler{ D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP };
		state.VertexBuffers[0] = m_GrassPlaneVB.get();
		state.RenderTargets[0] = m_FinalResult.get();
		state.DepthStencil = m_DepthTexture.get();
		context.ApplyState(state);
		context.CmdList->DrawInstanced(m_GrassPlaneVB->ByteSize / m_GrassPlaneVB->Stride, 1, 0, 0);

		GFX::Cmd::MarkerEnd(context);
	}

	// Prepare data for draw
	if(!m_Camera.FreezeFrustum)
	{
		GFX::Cmd::MarkerBegin(context, "Grass prepare");

		const uint32_t clearValue = 0;
		GFX::Cmd::UploadToBuffer(context, m_IndirectArgsCountBufferHP.get(), 0, &clearValue, 0, sizeof(uint32_t));
		GFX::Cmd::UploadToBuffer(context, m_IndirectArgsCountBufferLP.get(), 0, &clearValue, 0, sizeof(uint32_t));

		const uint32_t maxPatchInstances = GrassGenConfig.NumInstances / (GrassPatchSubdivision * GrassPatchSubdivision);

		ConstantBuffer cb{};
		for (uint32_t i = 0; i < 6; i++) cb.Add(m_Camera.CameraFrustum.Planes[i]);
		cb.Add(planeParams);
		cb.Add(GrassPatchSubdivision);
		cb.Add(maxPatchInstances);
		cb.Add(m_GrassMaterials[0].HighPoly.Mesh.PrimitiveCount);
		cb.Add(m_GrassMaterials[0].LowPoly.Mesh.PrimitiveCount);
		cb.Add(m_Camera.Position);
		cb.Add(0.0f); // Padding
		cb.Add(GrassPerfSettings);

		GraphicsState state{};
		state.Shader = m_GrassPrepareShader.get();
		state.ShaderStages = CS;
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.Table.SRVs[0] = m_GrassPatchDataBuffer.get();
		state.Table.UAVs[0] = m_IndirectArgsBufferHP.get();
		state.Table.UAVs[1] = m_IndirectArgsCountBufferHP.get();
		state.Table.UAVs[2] = m_IndirectArgsBufferLP.get();
		state.Table.UAVs[3] = m_IndirectArgsCountBufferLP.get();

		context.ApplyState(state);
		context.CmdList->Dispatch(MathUtility::CeilDiv(GrassPatchSubdivision * GrassPatchSubdivision, 128u), 1, 1);

		GFX::Cmd::MarkerEnd(context);
	}

	// Draw grass
	{
		GFX::Cmd::MarkerBegin(context, "Grass");

		D3D12_INDIRECT_ARGUMENT_DESC indirectArguments[5];
		indirectArguments[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		for (uint32_t i = 0; i < 4; i++)
		{
			indirectArguments[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			indirectArguments[i].Constant.DestOffsetIn32BitValues = i;
			indirectArguments[i].Constant.RootParameterIndex = 0;
			indirectArguments[i].Constant.Num32BitValuesToSet = 1;
		}

		ConstantBuffer cb{};
		cb.Add(m_Camera.ConstantData);
		cb.Add(GrassSettings);
		cb.Add(planeParams);
		cb.Add(SkyColor.ToXMF());

		GraphicsState state{};
		state.Shader = m_GrassShader.get();
		state.DepthStencilState.DepthEnable = true;
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.Table.SRVs[0] = m_GrassInstanceData.get();
		state.Table.SRVs[1] = m_WindTexture.get();
		state.Table.SRVs[2] = m_HeightMap.get();
		state.Table.SMPs[0] = Sampler{ D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP };
		state.RenderTargets[0] = m_FinalResult.get();
		state.DepthStencil = m_DepthTexture.get();
		state.PushConstantCount = 4;
		state.CommandSignature.ByteStride = INDIRECT_ARGUMENTS_STRIDE;
		state.CommandSignature.NumArgumentDescs = 5;
		state.CommandSignature.pArgumentDescs = indirectArguments;
		state.CommandSignature.NodeMask = 0;

		state.VertexBuffers[0] = m_GrassMaterials[0].HighPoly.Mesh.Positions;
		state.VertexBuffers[1] = m_GrassMaterials[0].HighPoly.Mesh.Texcoords;
		state.IndexBuffer = m_GrassMaterials[0].HighPoly.Mesh.Indices;
		ID3D12CommandSignature* commandSignature = context.ApplyState(state);
		GFX::Cmd::TransitionResource(context, m_IndirectArgsBufferHP.get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		GFX::Cmd::TransitionResource(context, m_IndirectArgsCountBufferHP.get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		context.CmdList->ExecuteIndirect(commandSignature, (UINT)MAX_INDIRECT_ARGUMENTS_COUNT, m_IndirectArgsBufferHP->Handle.Get(), 0, m_IndirectArgsCountBufferHP->Handle.Get(), 0);
		
		state.VertexBuffers[0] = m_GrassMaterials[0].LowPoly.Mesh.Positions;
		state.VertexBuffers[1] = m_GrassMaterials[0].LowPoly.Mesh.Texcoords;
		state.IndexBuffer = m_GrassMaterials[0].LowPoly.Mesh.Indices;
		commandSignature = context.ApplyState(state);
		GFX::Cmd::TransitionResource(context, m_IndirectArgsBufferLP.get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		GFX::Cmd::TransitionResource(context, m_IndirectArgsCountBufferLP.get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		context.CmdList->ExecuteIndirect(commandSignature, (UINT)MAX_INDIRECT_ARGUMENTS_COUNT, m_IndirectArgsBufferLP->Handle.Get(), 0, m_IndirectArgsCountBufferLP->Handle.Get(), 0);

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

	if (GrassAppGUI::GUIRequests.RegenerateGrass)
	{
		GrassAppGUI::GUIRequests.RegenerateGrass = false;
		RegenerateGrass(context);
	}

	if (GrassAppGUI::GUIRequests.ToggleWindTexture)
	{
		GrassAppGUI::GUIRequests.ToggleWindTexture = false;
		m_ShowWindTexture = !m_ShowWindTexture;
	}
}

void GrassApp::OnShaderReload(GraphicsContext& context)
{

}

void GrassApp::OnWindowResize(GraphicsContext& context)
{
	m_FinalResult = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::RTV));
	m_DepthTexture = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::DSV));
	m_Camera.AspectRatio = (float)AppConfig.WindowWidth / AppConfig.WindowHeight;
}

void GrassApp::RegenerateGrass(GraphicsContext& context)
{
	// Wait for context to finish before deleting the data
	ContextManager::Get().Flush();

	uint32_t minDataOffset = 0;
	uint32_t maxDataOffset = 0;

	// Generate grass instance data
	{
		struct GrassInstance
		{
			DirectX::XMFLOAT2 Position;
			DirectX::XMFLOAT3 Normal;
			float Height;
		};

		const uint32_t numInstancesPerPatch = GrassGenConfig.NumInstances / (GrassPatchSubdivision * GrassPatchSubdivision);
		const uint32_t numInstances = numInstancesPerPatch * 4;
		maxDataOffset = numInstances - numInstancesPerPatch;

		std::vector<GrassInstance> instanceData{};
		instanceData.resize(numInstances);
		for (uint32_t i = 0; i < numInstances; i++)
		{
			instanceData[i].Position = (Float2{ Random::SNorm(), Random::SNorm() } *Float2{ 0.5f, 0.5f }).ToXMF();
			instanceData[i].Normal = Float3(Random::SNorm(), 0.0f, Random::SNorm()).Normalize().ToXMF();
			instanceData[i].Height = Random::Float(GrassGenConfig.HeightRange.x, GrassGenConfig.HeightRange.y);
		}
		ResourceInitData initData{ &context, instanceData.data() };
		m_GrassInstanceData =  ScopedRef<Buffer>(GFX::CreateBuffer((uint32_t)instanceData.size() * sizeof(GrassInstance), sizeof(GrassInstance), RCF::None, &initData));
	}

	// Generate patch data
	{
		std::vector<GrassPatchData> patchData{};
		patchData.resize(GrassPatchSubdivision * GrassPatchSubdivision);
		for (uint32_t i = 0; i < GrassPatchSubdivision * GrassPatchSubdivision; i++)
		{
			patchData[i].InstanceDataOffset = Random::UInt(minDataOffset, maxDataOffset);
		}
		GFX::Cmd::UploadToBuffer(context, m_GrassPatchDataBuffer.get(), 0, patchData.data(), 0, sizeof(GrassPatchData) * GrassPatchSubdivision * GrassPatchSubdivision);
	}
}
