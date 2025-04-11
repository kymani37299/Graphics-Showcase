#include "VolumetricLightsApp.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Texture.h>
#include <Engine/Render/Shader.h>
#include <Engine/Utility/MathUtility.h>

#include "Common/ConstantBuffer.h"
#include "VolumetricLights/VolumetricLightsAppGUI.h"
#include "VolumetricLights/Settings.h"

static Float3 GetDirLight(const ModelLoading::Scene& scene)
{
	Float3 dirLight{ 0.5f, 0.5f, 0.3f };
	for (const auto& light : scene.Lights)
	{
		if (light.Type == ModelLoading::SceneLight::LightType::Directional)
		{
			// dirLight = light.Direction;
		}
	}
	return dirLight;
}

void VolumetricLightsApp::OnInit(GraphicsContext& context)
{
	m_BackgroundShader = ScopedRef<Shader>{ new Shader{"Application/VolumetricLights/Shaders/background.hlsl"} };
	m_SceneShader = ScopedRef<Shader>{ new Shader{"Application/VolumetricLights/Shaders/scene.hlsl"} };
	m_ShadowShader = ScopedRef<Shader>{ new Shader{"Application/VolumetricLights/Shaders/shadowmap.hlsl"} };
	m_VolumetricFogShader = ScopedRef<Shader>{ new Shader{"Application/VolumetricLights/Shaders/volumetric_fog.hlsl"} };

	ModelLoading::Loader loader{ context };
	m_Scene = loader.Load("Application/VolumetricLights/Resources/scene.gltf");

	if (!m_Scene.Cameras.empty())
	{
		m_Camera.Position = m_Scene.Cameras[0].Position;
		m_Camera.Rotation = m_Scene.Cameras[0].Rotation.ToEuler();
	}

	m_Shadowmap = ScopedRef<Texture>(GFX::CreateTexture(1024u, 1024u, RCF::DSV));

	VolumetricLightsAppGUI::AddGUI();
	OnWindowResize(context);
}

void VolumetricLightsApp::OnDestroy(GraphicsContext& context)
{
	ModelLoading::Free(m_Scene);
	VolumetricLightsAppGUI::RemoveGUI();
}

Texture* VolumetricLightsApp::OnDraw(GraphicsContext& context)
{
	GFX::Cmd::ClearDepthStencil(context, m_DepthTexture.get());

	const Float3 dirLight = GetDirLight(m_Scene);
	Camera shadowCamera = Camera::CreateOrtho(100.0f, 100.0f, -100.0f, 100.0f);
	shadowCamera.UseRotation = false;
	shadowCamera.Position = m_Camera.Position;
	shadowCamera.Forward = -1.0f * dirLight;
	shadowCamera.UpdateConstantData();

	// Background
	{
		PROFILE_SECTION(context, "Background");

		ConstantBuffer cb{};
		cb.Add(Float3{0.2f, 0.15f, 0.7f});

		GraphicsState state{};
		state.Shader = m_BackgroundShader.get();
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.RenderTargets[0] = m_FinalResult.get();
		GFX::Cmd::DrawFC(context, state);
	}

	// Shadowmap
	{
		PROFILE_SECTION(context, "Shadowmap");

		GFX::Cmd::ClearDepthStencil(context, m_Shadowmap.get());

		ConstantBuffer cb{};
		cb.Add(shadowCamera.ConstantData);
		
		GraphicsState state{};
		state.Shader = m_ShadowShader.get();
		state.ShaderStages = VS;
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.DepthStencil = m_Shadowmap.get();
		state.DepthStencilState.DepthEnable = true;

		for (const auto& object : m_Scene.Objects)
		{
			ConstantBuffer objectCB{};
			objectCB.Add(XMUtility::ToHLSLFloat4x4(object.ModelToWorld));

			state.Table.CBVs[1] = objectCB.GetBuffer(context);
			state.VertexBuffers[0] = object.Mesh.Positions;
			state.IndexBuffer = object.Mesh.Indices;

			context.ApplyState(state);
			GFX::Cmd::DrawIndexed(context, object.Mesh.PrimitiveCount, 0, 0);
		}
	}

	// Render scene
	{
		PROFILE_SECTION(context, "Render scene");

		ConstantBuffer cb{};
		cb.Add(m_Camera.ConstantData);
		cb.Add(shadowCamera.ConstantData);
		cb.Add(dirLight);

		GraphicsState state{};
		state.Shader = m_SceneShader.get();
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.Table.SRVs[0] = m_Shadowmap.get();
		state.Table.SMPs[0] = Sampler{ D3D12_FILTER_MIN_MAG_MIP_POINT , D3D12_TEXTURE_ADDRESS_MODE_WRAP };
		state.RenderTargets[0] = m_FinalResult.get();
		state.DepthStencil = m_DepthTexture.get();
		state.DepthStencilState.DepthEnable = true;
		
		for (const auto& object : m_Scene.Objects)
		{
			DirectX::XMMATRIX mat = DirectX::XMLoadFloat4x4(&object.ModelToWorld);
			mat = DirectX::XMMatrixInverse(nullptr, mat);
			mat = DirectX::XMMatrixTranspose(mat);
			DirectX::XMFLOAT4X4 modelToWorldNormal;
			DirectX::XMStoreFloat4x4(&modelToWorldNormal, mat);

			ConstantBuffer objectCB{};
			objectCB.Add(XMUtility::ToHLSLFloat4x4(object.ModelToWorld));
			objectCB.Add(XMUtility::ToHLSLFloat4x4(modelToWorldNormal));
			objectCB.Add(object.Material.AlbedoFactor);
			
			state.Table.CBVs[1] = objectCB.GetBuffer(context);
			state.VertexBuffers[0] = object.Mesh.Positions;
			state.VertexBuffers[1] = object.Mesh.Normals;
			state.IndexBuffer = object.Mesh.Indices;
			
			context.ApplyState(state);
			GFX::Cmd::DrawIndexed(context, object.Mesh.PrimitiveCount, 0, 0);
		}
	}

	// Volumetric fog
	{
		PROFILE_SECTION(context, "Volumetric fog");

		ConstantBuffer cb{};
		cb.Add(m_Camera.ConstantData);
		cb.Add(shadowCamera.ConstantData);
		cb.Add(dirLight);

		GraphicsState state{};
		state.Shader = m_VolumetricFogShader.get();
		state.Table.CBVs[0] = cb.GetBuffer(context);
		state.Table.SMPs[0] = Sampler{ D3D12_FILTER_MIN_MAG_MIP_POINT , D3D12_TEXTURE_ADDRESS_MODE_WRAP };
		state.Table.SRVs[0] = m_DepthTexture.get();
		state.Table.SRVs[1] = m_Shadowmap.get();
		state.RenderTargets[0] = m_FinalResult.get();
		state.BlendState.RenderTarget[0].BlendEnable = true;

		GFX::Cmd::DrawFC(context, state);
	}

	return m_FinalResult.get();
}

void VolumetricLightsApp::OnUpdate(GraphicsContext& context, float dt)
{
	m_Camera.Update(dt);
}

void VolumetricLightsApp::OnShaderReload(GraphicsContext& context)
{

}

void VolumetricLightsApp::OnWindowResize(GraphicsContext& context)
{
	m_Camera.AspectRatio = (float)AppConfig.WindowWidth / AppConfig.WindowHeight;
	m_FinalResult = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::RTV));
	m_DepthTexture = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::DSV));
}