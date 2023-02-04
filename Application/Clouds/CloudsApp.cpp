#include "CloudsApp.h"

#include <Engine/System/ApplicationConfiguration.h>
#include <Engine/Render/Commands.h>
#include <Engine/Render/Context.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Texture.h>
#include <Engine/Render/Shader.h>
#include <Engine/System/Input.h>
#include <Engine/Utility/Random.h>

#include "Common/ConstantBuffer.h"
#include "Clouds/CloudsAppGUI.h"
#include "Clouds/Settings.h"

CloudsSettingsCB CloudSettings;
SunSettingsCB SunSettings;
CloudNoiseSettingsStruct CloudNoiseSettings;

static Texture* GenerateWorleyNoise(GraphicsContext& context)
{
	GFX::Cmd::MarkerBegin(context, "Generate Worley Noise");

	struct WorleyNoiseSamplePoint
	{
		DirectX::XMFLOAT3 Points[4];
	};

	// Generate random points
	std::vector<WorleyNoiseSamplePoint> points{};
	points.resize(CloudNoiseSettings.NumSamplePoints);
	for (uint32_t i = 0; i < CloudNoiseSettings.NumSamplePoints; i++)
	{
		for (uint32_t j = 0; j < 4; j++)
		{
			points[i].Points[j].x = Random::UNorm();
			points[i].Points[j].y = Random::UNorm();
			points[i].Points[j].z = Random::UNorm();
		}
	}

	// Create resources
	Shader* worleyShader = new Shader("Application/Clouds/Shaders/worley_noise.hlsl");
	ResourceInitData initData{&context, points.data()};
	Buffer* pointsBuffer = GFX::CreateBuffer(CloudNoiseSettings.NumSamplePoints * sizeof(WorleyNoiseSamplePoint), sizeof(WorleyNoiseSamplePoint), RCF::None, &initData);
	Texture* outputTexture = GFX::CreateTexture3D(CloudNoiseSettings.VolumeWidth, CloudNoiseSettings.VolumeHeight, CloudNoiseSettings.VolumeDepth, RCF::UAV);

	// Record commands
	ConstantBuffer cbData{};
	cbData.Add(CloudNoiseSettings.NumSamplePoints);
	cbData.Add((float) CloudNoiseSettings.VolumeWidth);
	cbData.Add((float) CloudNoiseSettings.VolumeHeight);
	cbData.Add((float) CloudNoiseSettings.VolumeDepth);
	cbData.Add((float) CloudNoiseSettings.NormalizationFactor);

	GraphicsState state{};
	state.Shader = worleyShader;
	state.ShaderStages = CS;
	state.Table.CBVs[0] = cbData.GetBuffer(context);
	state.Table.SRVs[0] = pointsBuffer;
	state.Table.UAVs[0] = outputTexture;
	context.ApplyState(state);

	static constexpr uint32_t NumWavesPerDim = 8;
	context.CmdList->Dispatch(
		MathUtility::CeilDiv(CloudNoiseSettings.VolumeWidth, NumWavesPerDim), 
		MathUtility::CeilDiv(CloudNoiseSettings.VolumeHeight, NumWavesPerDim),
		MathUtility::CeilDiv(CloudNoiseSettings.VolumeDepth, NumWavesPerDim));

	GFX::Cmd::Delete(context, pointsBuffer);
	GFX::Cmd::Delete(context, worleyShader);

	GFX::Cmd::MarkerEnd(context);

	return outputTexture;
}

void CloudsApp::OnInit(GraphicsContext& context)
{
	m_CloudsShader = ScopedRef<Shader>(new Shader("Application/Clouds/Shaders/clouds.hlsl"));
	
	m_Camera.Position = Float3(0.0f, 0.0f, 0.0f);
	m_Camera.Rotation = Float3(1.0f, -37.0f, 0.0f);

	CloudsAppGUI::AddGUI(this);
	OnShaderReload(context);
	OnWindowResize(context);
}

void CloudsApp::OnDestroy(GraphicsContext& context)
{
	CloudsAppGUI::RemoveGUI();
}

Texture* CloudsApp::OnDraw(GraphicsContext& context)
{
	GFX::Cmd::MarkerBegin(context, "Clouds");

	ConstantBuffer cb{};
	cb.Add(CloudSettings);
	cb.Add(SunSettings);
	cb.Add(m_Camera.ConstantData);

	GraphicsState state{};
	state.Shader = m_CloudsShader.get();
	state.Table.CBVs[0] = cb.GetBuffer(context);
	state.Table.SRVs[0] = m_CloudNoise.get();
	state.Table.SRVs[1] = m_CloudDetailNoise.get();
	state.Table.SMPs[0] = Sampler{ D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP };
	state.RenderTargets[0] = m_FinalResult.get();
	GFX::Cmd::DrawFC(context, state);

	GFX::Cmd::MarkerEnd(context);

	return m_FinalResult.get();
}

void CloudsApp::OnUpdate(GraphicsContext& context, float dt)
{
	m_Camera.Update(dt);

	CloudSettings.SamplingOffset += (dt / 1000.0f) * 0.05f;
	CloudSettings.SamplingDetailOffset += (dt / 1000.0f) * 0.12f;
}

void CloudsApp::OnShaderReload(GraphicsContext& context)
{
	m_CloudNoise = ScopedRef<Texture>(GenerateWorleyNoise(context));
	m_CloudDetailNoise = ScopedRef<Texture>(GenerateWorleyNoise(context));
}

void CloudsApp::OnWindowResize(GraphicsContext& context)
{
	m_FinalResult = ScopedRef<Texture>(GFX::CreateTexture(AppConfig.WindowWidth, AppConfig.WindowHeight, RCF::RTV));
	m_Camera.AspectRatio = (float)AppConfig.WindowWidth / AppConfig.WindowHeight;
}
