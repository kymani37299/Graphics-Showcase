#include "MeshletsApp.h"

#include <DirectXMesh/DirectXMesh.h>

#include <Engine/Render/Commands.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Texture.h>
#include <Engine/Render/Shader.h>
#include <Engine/Loading/ModelLoading.h>
#include <Engine/System/Input.h>
#include <Engine/Utility/Random.h>

#include "Common/ConstantBuffer.h"
#include "Meshlets/Settings.h"
#include "Meshlets/MeshletsAppGUI.h"

static uint32_t constexpr SampleSceneObjectIndex = 1;

void MeshletsApp::OnInit(GraphicsContext& context)
{
	MeshletsAppGUI::AddGUI();
	OnWindowResize(context);

	ModelLoading::Loader loader{ context };
	m_Scene = loader.Load("Application/Meshlets/Resources/Dragon/DragonAttenuation.gltf");
	ASSERT_CORE(m_Scene.Objects.size() > SampleSceneObjectIndex, "Invalid sample scene for model loading in MeshletsApp!");

	auto mesh = m_Scene.Objects[SampleSceneObjectIndex].Mesh;
	
	std::vector<DirectX::Meshlet> meshlets;
	std::vector<uint8_t> uniqueVertexIB;
	std::vector<DirectX::MeshletTriangle> meshletTriangles;
	DirectX::ComputeMeshlets(mesh.IndicesData.data(), mesh.IndicesData.size() / 3, (const DirectX::XMFLOAT3*)mesh.PositionsData.data(), mesh.PositionsData.size(), nullptr, meshlets, uniqueVertexIB, meshletTriangles);

	{
		ResourceInitData initData{ &context , meshlets.data() };
		m_MeshletsBuffer = ScopedRef<Buffer>(GFX::CreateBuffer(sizeof(DirectX::Meshlet) * static_cast<uint32_t>(meshlets.size()), sizeof(DirectX::Meshlet), RCF::None, &initData));
	}

	{
		ResourceInitData initData{ &context , uniqueVertexIB.data() };
		m_UniqueVertexIB = ScopedRef<Buffer>(GFX::CreateBuffer(static_cast<uint32_t>(uniqueVertexIB.size()), 1, RCF::RAW, &initData));
	}

	{
		ResourceInitData initData{ &context , meshletTriangles.data() };
		m_MeshletsTriangleBuffer = ScopedRef<Buffer>(GFX::CreateBuffer(static_cast<uint32_t>(meshletTriangles.size()) * sizeof(DirectX::MeshletTriangle), sizeof(DirectX::MeshletTriangle), RCF::None, &initData));
	}

	m_Shader = ScopedRef<Shader>(new Shader("Application/Meshlets/Shaders/draw.hlsl"));
}

void MeshletsApp::OnDestroy(GraphicsContext& context)
{
	MeshletsAppGUI::RemoveGUI();
	ModelLoading::Free(m_Scene);
}

Texture* MeshletsApp::OnDraw(GraphicsContext& context)
{
	GFX::Cmd::ClearRenderTarget(context, m_FinalResult.get());
	GFX::Cmd::ClearDepthStencil(context, m_DepthTexture.get());

	ConstantBuffer cb{};
	cb.Add(m_Camera.ConstantData);

	auto mesh = m_Scene.Objects[SampleSceneObjectIndex].Mesh;

	GraphicsState state{};
	state.Shader = m_Shader.get();
	state.ShaderStages = MS | PS;
	state.Table.CBVs[0] = cb.GetBuffer(context);
	state.Table.SRVs[0] = mesh.Positions;
	state.Table.SRVs[1] = mesh.Normals;
	state.Table.SRVs[2] = m_MeshletsBuffer.get();
	state.Table.SRVs[3] = m_MeshletsTriangleBuffer.get();
	state.Table.SRVs[4] = m_UniqueVertexIB.get();
	state.RenderTargets[0] = m_FinalResult.get();
	state.DepthStencil = m_DepthTexture.get();
	state.DepthStencilState.DepthEnable = true;

	context.ApplyState(state);
	context.CmdList->DispatchMesh(m_MeshletsBuffer->ByteSize / m_MeshletsBuffer->Stride, 1, 1);
	
	return m_FinalResult.get();
}

void MeshletsApp::OnUpdate(GraphicsContext& context, float dt)
{
	m_Camera.Update(dt);
	
	if (Input::IsKeyJustPressed('C'))
	{
		m_Camera.FreezeFrustum = !m_Camera.FreezeFrustum;
	}
}

void MeshletsApp::OnShaderReload(GraphicsContext& context)
{

}

void MeshletsApp::OnWindowResize(GraphicsContext& context)
{
	m_FinalResult = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::RTV));
	m_DepthTexture = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::DSV));
	m_Camera.AspectRatio = (float)AppConfig.WindowWidth / AppConfig.WindowHeight;
}