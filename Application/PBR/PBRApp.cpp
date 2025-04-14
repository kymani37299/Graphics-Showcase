#include "PBRApp.h"

#include <Engine/System/ApplicationConfiguration.h>
#include <Engine/Render/Commands.h>
#include <Engine/Render/Context.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Texture.h>
#include <Engine/Render/Shader.h>
#include <Engine/System/Input.h>
#include <Engine/Utility/Random.h>

#include "Common/ConstantBuffer.h"
#include "PBR/PBRAppGUI.h"
#include "PBR/Settings.h"

struct PBRSettingsCB
{
	Float3 SubsurfaceAlbedo; float Pad0;
	Float3 LightDir_LightPos; float Pad1;
	Float3 LightColor; float Pad2;
	Float3 F0; float Pad3;
	Float3 F90; float Pad4;
	float P; Float3 Pad5;
};

static void FillPBRSettings(PBRSettingsCB& pbrSettings, const PBRConfig& pbrCfg)
{
	pbrSettings.SubsurfaceAlbedo = pbrCfg.SubsurfaceAlbedo;
	switch (pbrCfg.Illumination_Type)
	{
	case Illumination::None:
		pbrSettings.LightDir_LightPos = pbrCfg.DirectionalLight;
		break;
	case Illumination::Directional:
		pbrSettings.LightDir_LightPos = pbrCfg.DirectionalLight;
		break;
	case Illumination::Point:
		pbrSettings.LightDir_LightPos = pbrCfg.PointLight;
		break;
	}
	pbrSettings.LightColor = pbrCfg.LightColor;

	pbrSettings.F0 = pbrCfg.F0;
	pbrSettings.F90 = pbrCfg.F90;
	pbrSettings.P = pbrCfg.P;
}

static void FillShaderConfig(std::vector<std::string>& shaderConfig, const PBRConfig& pbrCfg)
{
	if (pbrCfg.BRDF_Function == BRDF::Lambert)
	{
		shaderConfig.push_back("BRDF_Lambert");
	}

	switch (pbrCfg.Illumination_Type)
	{
	case Illumination::Directional:
		shaderConfig.push_back("Illumination_Directional");
		break;
	case Illumination::Point:
		shaderConfig.push_back("Illumination_Point");
		break;
	}

	switch (pbrCfg.FresnelReflectance)
	{
	case FresnelReflectance::Shlick:
		shaderConfig.push_back("Fresnel_Shlick");
		break;
	}
}

PBRConfig PBRCfg;

void PBRApp::OnInit(GraphicsContext& context)
{
	m_PBRShader = ScopedRef<Shader>(new Shader("Application/PBR/Shaders/pbr.hlsl"));
	m_BackgroundShader = ScopedRef<Shader>(new Shader("Application/Animation/background.hlsl"));

	m_Camera.Position = Float3(7.5f, 3.0f, 0.0f);
	m_Camera.Rotation = Float3(-0.3f, -34.6f, 0.0f);

	ModelLoading::Loader loader{ context };
	m_Scene = loader.Load("Application/PBR/Resources/pbr_scene.gltf");

	PBRAppGUI::AddGUI(this);
	OnShaderReload(context);
	OnWindowResize(context);
}

void PBRApp::OnDestroy(GraphicsContext& context)
{
	PBRAppGUI::RemoveGUI();
}

Texture* PBRApp::OnDraw(GraphicsContext& context)
{
	GFX::Cmd::MarkerBegin(context, "PBR");

	GFX::Cmd::ClearRenderTarget(context, m_FinalResult.get());
	GFX::Cmd::ClearDepthStencil(context, m_DepthTexture.get());

	// Background
	{
		static const Float3 SkyColor{ 0.3f, 0.3f, 0.3f };

		GFX::Cmd::MarkerBegin(context, "Background");

		ConstantBuffer cb{};
		cb.Add(SkyColor);

		GraphicsState state{};
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.Shader = m_BackgroundShader.get();
		state.RenderTargets[0] = m_FinalResult.get();
		GFX::Cmd::DrawFC(context, state);
		context.ApplyState(state);
		GFX::Cmd::MarkerEnd(context);
	}

	PBRSettingsCB settingsCB{};
	FillPBRSettings(settingsCB, PBRCfg);

	GraphicsState state{};
	state.Shader = m_PBRShader.get();
	FillShaderConfig(state.ShaderConfig, PBRCfg);
	
	state.RenderTargets[0] = m_FinalResult.get();
	state.DepthStencil = m_DepthTexture.get();
	state.DepthStencilState.DepthEnable = true;
	
	for (const ModelLoading::SceneObject& object : m_Scene.Objects)
	{
		static const float RotationSpeedNormalizer = 0.0001f;
		const DirectX::XMMATRIX modelRotationMatrix = DirectX::XMMatrixRotationY(m_TimeSinceStarted * PBRCfg.ModelRotationSpeed * RotationSpeedNormalizer);
		const DirectX::XMMATRIX rotatedModelToWorld = modelRotationMatrix * DirectX::XMLoadFloat4x4(&object.ModelToWorld);
		
		ConstantBuffer cb{};
		cb.Add(settingsCB);
		cb.Add(m_Camera.ConstantData);
		cb.Add(XMUtility::ToHLSLFloat4x4(rotatedModelToWorld));

		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.VertexBuffers[0] = object.Mesh.Positions;
		state.VertexBuffers[1] = object.Mesh.Normals;
		state.IndexBuffer = object.Mesh.Indices;
		context.ApplyState(state);
		context.CmdList->DrawIndexedInstanced(object.Mesh.PrimitiveCount, 1, 0, 0, 0);
	}

	GFX::Cmd::MarkerEnd(context);

	return m_FinalResult.get();
}

void PBRApp::OnUpdate(GraphicsContext& context, float dt)
{
	m_Camera.Update(dt);
	m_TimeSinceStarted += dt;
}

void PBRApp::OnShaderReload(GraphicsContext& context)
{
}

void PBRApp::OnWindowResize(GraphicsContext& context)
{
	m_DepthTexture = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::DSV));
	m_FinalResult = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::RTV));
	m_Camera.AspectRatio = (float)AppConfig.WindowWidth / AppConfig.WindowHeight;
}
