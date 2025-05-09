#include "CloudsAppGUI.h"

#include <Engine/Render/Device.h>
#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui_Core.h>

#include "Clouds/CloudsApp.h"
#include "Clouds/Settings.h"

namespace CloudsAppGUI
{
	class CloudSettingsGUI : public GUIElement
	{
	public:
		CloudSettingsGUI() : GUIElement("Cloud settings", GUIFlags::None) {}
		void Update(float dt) override {}

		void Render(GraphicsContext& context) override
		{
			ImGui::Text("Cloud box");
			ImGui::DragFloat("Position", CloudSettings.Position);
			ImGui::DragFloat("Size", CloudSettings.Size);

			ImGui::Separator();

			ImGui::Text("Sampling");
			if (ImGui::DragFloat("Weights", CloudSettings.SamplingWeights, 0.1f))
			{
				CloudSettings.SamplingWeights.x = MAX(CloudSettings.SamplingWeights.x, 0.0f);
				CloudSettings.SamplingWeights.y = MAX(CloudSettings.SamplingWeights.y, 0.0f);
				CloudSettings.SamplingWeights.z = MAX(CloudSettings.SamplingWeights.z, 0.0f);
				CloudSettings.SamplingWeights.w = MAX(CloudSettings.SamplingWeights.w, 0.0f);
				CloudSettings.SamplingWeights = Float4{ CloudSettings.SamplingWeights }.NormalizeFast().ToXMF();
			}
			if (ImGui::DragFloat("Detail Weights", CloudSettings.SamplingDetailWeights, 0.1f))
			{
				CloudSettings.SamplingDetailWeights.x = MAX(CloudSettings.SamplingDetailWeights.x, 0.0f);
				CloudSettings.SamplingDetailWeights.y = MAX(CloudSettings.SamplingDetailWeights.y, 0.0f);
				CloudSettings.SamplingDetailWeights.z = MAX(CloudSettings.SamplingDetailWeights.z, 0.0f);
				CloudSettings.SamplingDetailWeights.w = MAX(CloudSettings.SamplingDetailWeights.w, 0.0f);
				CloudSettings.SamplingDetailWeights = Float4{ CloudSettings.SamplingDetailWeights }.NormalizeFast().ToXMF();
			}
			ImGui::PushItemWidth(100);
			ImGui::DragFloat("Scale", &CloudSettings.SamplingScale, 0.1f);
			ImGui::DragFloat("Offset", &CloudSettings.SamplingOffset, 0.1f);
			ImGui::DragFloat("Detail scale", &CloudSettings.SamplingDetailScale, 0.1f);
			ImGui::DragFloat("Detail offset", &CloudSettings.SamplingDetailOffset, 0.1f);

			ImGui::Separator();

			ImGui::Text("Lighting");
			ImGui::DragFloat("Density treshold", &CloudSettings.DensityTreshold, 0.1f);
			ImGui::DragFloat("Density multiplier", &CloudSettings.DensityMultiplier, 0.1f);
			ImGui::DragFloat("Detail density multiplier", &CloudSettings.DetailDensityMultiplier, 0.1f);
			ImGui::DragFloat("Cloud light absorption", &CloudSettings.CloudLightAbsorption, 0.1f);
			ImGui::DragFloat("Sun light absorption", &CloudSettings.SunLightAbsorption, 0.1f);
			ImGui::DragFloat("Sun light bias", &CloudSettings.SunLightBias, 0.1f);
			ImGui::DragFloat("Sun phase value", &CloudSettings.SunPhaseValue, 0.1f);

			ImGui::Separator();

			ImGui::Text("Raymaching");
			ImGui::DragFloat("CloudMarch step size", &CloudSettings.CloudMarchStepSize, 0.01f, 0.01f);
			ImGui::DragFloat("LightMarch step size", &CloudSettings.LightMarchStepSize, 0.01f, 0.01f);
		
			ImGui::PopItemWidth();
		}
	};

	class SunSettingsGUI : public GUIElement
	{
	public:
		SunSettingsGUI() : GUIElement("Sun settings", GUIFlags::None) {}
		void Update(float dt) override {}

		void Render(GraphicsContext& context) override
		{
			ImGui::DragFloat("Position", SunSettings.Position);
			ImGui::ColorEdit("Radiance", SunSettings.Radiance);
		}
	};

	struct CloudNoiseSettingsGUI : public GUIElement
	{
	public:
		CloudNoiseSettingsGUI(CloudsApp* cloudsApp) : GUIElement("Cloud noise settings", GUIFlags::None)
		{
			m_App = cloudsApp;
		}
		void Update(float dt) override {}

		void Render(GraphicsContext& context) override
		{
			if (ImGui::Button("Regenerate noise"))
			{
				m_App->OnShaderReload(context);
			}
			ImGui::PushItemWidth(100);
			ImGui::DragUint("Number of sample points", CloudNoiseSettings.NumSamplePoints);
			ImGui::DragFloat("Normalization factor", &CloudNoiseSettings.NormalizationFactor, 0.1f);
			ImGui::DragUint("Volume width", CloudNoiseSettings.VolumeWidth);
			ImGui::DragUint("Volume height", CloudNoiseSettings.VolumeHeight);
			ImGui::DragUint("Volume depth", CloudNoiseSettings.VolumeDepth);
			ImGui::PopItemWidth();
		}
	private:
		CloudsApp* m_App;

	};

	void AddGUI(CloudsApp* cloudsApp)
	{
		GUI* gui = GUI::Get();
		gui->PushMenu("Clouds");
		gui->AddElement(new CloudSettingsGUI{});
		gui->AddElement(new SunSettingsGUI{});
		gui->AddElement(new CloudNoiseSettingsGUI{cloudsApp});
		gui->PopMenu();
	}

	void RemoveGUI()
	{
		GUI::Get()->RemoveMenu("Clouds");
	}
}