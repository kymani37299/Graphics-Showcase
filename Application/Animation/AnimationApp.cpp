#include "AnimationApp.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Texture.h>
#include <Engine/Render/Shader.h>
#include <Engine/Loading/ModelLoading.h>
#include <Engine/Loading/TextureLoading.h>
#include <Engine/Loading/AnimationOperations.h>
#include <Engine/Utility/MathUtility.h>

#include "Animation/AnimationAppGUI.h"
#include "Common/ConstantBuffer.h"

void AnimationApp::OnInit(GraphicsContext& context)
{
	ModelLoading::Loader loader{ context };
	m_Scene = loader.Load("Application/Animation/Resources/scene.gltf");
	
	m_GeometryShader = ScopedRef<Shader>(new Shader{ "Application/Animation/geometry.hlsl" });
	m_BackgroundShader = ScopedRef<Shader>(new Shader("Application/Animation/background.hlsl"));
	m_EmptyBuffer = ScopedRef<Buffer>(GFX::CreateBuffer(1, 1, RCF::None));

	m_Camera.Position = Float3(0.5f, 0.7f, 15.0f);
	m_Camera.Rotation = Float3(-0.15f, -1.6f, 0.0f);

	AnimationAppGUI::AddGUI(this);
	OnWindowResize(context);
}

void AnimationApp::OnDestroy(GraphicsContext& context)
{
	for (ModelLoading::SceneObject& obj : m_Scene.Objects)
	{
		ModelLoading::Free(context, obj);
	}
	AnimationAppGUI::RemoveGUI();
}

Texture* AnimationApp::OnDraw(GraphicsContext& context)
{
	GFX::Cmd::ClearRenderTarget(context, m_FinalResult.get());
	GFX::Cmd::ClearDepthStencil(context, m_DepthTexture.get());

	// Background
	{
		static const Float3 SkyColor = Float3(135.0f, 206.0f, 235.0f) / 255.0f;

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

	GFX::Cmd::MarkerBegin(context, "Geometry");

	GraphicsState state{};
	state.RenderTargets[0] = m_FinalResult.get();
	state.DepthStencil = m_DepthTexture.get();
	state.DepthStencilState.DepthEnable = true;
	state.Table.SMPs[0] = Sampler{ D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP };
	
	for (const ModelLoading::SceneObject& object : m_Scene.Objects)
	{
		state.ShaderConfig.clear();
		
		DirectX::XMFLOAT4X4 animationTransform = AnimationOperations::GetAnimationTransformation(object.AnimcationData, m_AnimationTime, AnimationOperations::AnimationType::Repeat);
		
		ConstantBuffer cb{};
		cb.Add(m_Camera.ConstantData);
		cb.Add(XMUtility::ToHLSLFloat4x4(animationTransform));
		cb.Add(XMUtility::ToHLSLFloat4x4(object.ModelToWorld));
		cb.Add(object.Material.AlbedoFactor);

		state.Table.SRVs[0] = object.Material.Albedo;
		state.VertexBuffers[0] = object.Mesh.Positions;
		state.VertexBuffers[1] = object.Mesh.Texcoords;
		state.VertexBuffers[2] = object.Mesh.Normals;
		state.IndexBuffer = object.Mesh.Indices;
		state.Shader = m_GeometryShader.get();

		// Object is using morphs
		if (!object.MorphTargets.empty())
		{
			// Must match with geometry.hlsl
			constexpr uint32_t MaxMorphWeights = 16;

			state.ShaderConfig.push_back("APPLY_MORPHS");

			std::vector<float> morphWeights = {};
			if(m_EnableWeightAnimation)
				morphWeights = AnimationOperations::GetAnimatedMorphWeights(object, m_AnimationTime, AnimationOperations::AnimationType::PingPong);
			else
			{
				for (const ModelLoading::MorphTarget& target : object.MorphTargets)
					morphWeights.push_back(target.Weight);
			}
			
			const uint32_t numWeights = min((uint32_t) morphWeights.size(), MaxMorphWeights);
			
			cb.Add(numWeights);
			const uint32_t morphTargetsBinding = 1;
			for (uint32_t i = 0; i < MaxMorphWeights; i++)
			{
				cb.Add(i < numWeights ? morphWeights[i] : 0.0f);
				state.Table.SRVs[i+ morphTargetsBinding] = i < numWeights ? object.MorphTargets[i].Data : m_EmptyBuffer.get();
			}
		}

		// Object is using skinning
		if (!object.Skeleton.empty())
		{
			// Must match with geometry.hlsl
			constexpr uint32_t MaxSkeletonJoints = 128;
			ASSERT(object.Skeleton.size() < MaxSkeletonJoints, "Too much skeleton joints!");

			state.ShaderConfig.push_back("APPLY_SKIN");
			state.VertexBuffers[3] = object.Mesh.Weights;
			state.VertexBuffers[4] = object.Mesh.Joints;

			std::vector<DirectX::XMFLOAT4X4> jointTransforms;
			std::vector<DirectX::XMFLOAT4X4> jointAnimationTransformations;
			std::vector<DirectX::XMFLOAT4X4> modelToJointTransforms;
			jointTransforms.resize(MaxSkeletonJoints);
			jointAnimationTransformations.resize(MaxSkeletonJoints);
			modelToJointTransforms.resize(MaxSkeletonJoints);

			for (size_t i = 0; i < min(MaxSkeletonJoints, object.Skeleton.size()); i++)
			{
				const ModelLoading::SkeletonJoint& joint = object.Skeleton[i];

				jointTransforms[i] = joint.Transform;
				jointAnimationTransformations[i] = AnimationOperations::GetAnimationTransformation(joint.Animations, m_AnimationTime, AnimationOperations::AnimationType::Repeat);
				modelToJointTransforms[i] = joint.ModelToJoint;
			}

			for (uint32_t i = 0; i < MaxSkeletonJoints; i++) cb.Add(XMUtility::ToHLSLFloat4x4(jointTransforms[i]));
			for (uint32_t i = 0; i < MaxSkeletonJoints; i++) cb.Add(XMUtility::ToHLSLFloat4x4(jointAnimationTransformations[i]));
			for (uint32_t i = 0; i < MaxSkeletonJoints; i++) cb.Add(XMUtility::ToHLSLFloat4x4(modelToJointTransforms[i]));
		}

		state.Table.CBVs[0] = cb.GetBuffer(context);
		context.ApplyState(state);
		context.CmdList->DrawIndexedInstanced(object.Mesh.PrimitiveCount, 1, 0, 0, 0);
	}

	GFX::Cmd::MarkerEnd(context);
	return m_FinalResult.get();
}

void AnimationApp::OnUpdate(GraphicsContext& context, float dt)
{
	m_Camera.Update(dt);

	constexpr float animationSpeed = 1.0f;
	m_AnimationTime += dt * animationSpeed / 1000.0f;
}

void AnimationApp::OnShaderReload(GraphicsContext& context)
{

}

void AnimationApp::OnWindowResize(GraphicsContext& context)
{
	m_FinalResult = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::RTV));
	m_DepthTexture = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::DSV));
	m_Camera.AspectRatio = (float)AppConfig.WindowWidth / AppConfig.WindowHeight;
}