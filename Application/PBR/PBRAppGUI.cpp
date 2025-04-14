#include "PBRAppGUI.h"

#include <Engine/Render/Device.h>
#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui_Core.h>

#include "PBR/PBRApp.h"
#include "PBR/Settings.h"

namespace PBRAppGUI
{
	template<typename EnumClass>
	void EnumSelect(const char* label, EnumClass& currentValue)
	{
		const char* currentValueStr = ToString(currentValue);

		if (ImGui::BeginCombo(label, currentValueStr))
		{
			for (int i = 0; i < static_cast<uint32_t>(EnumClass::Count); i++)
			{
				const EnumClass item = static_cast<EnumClass>(i);
				const bool isSelected = (item == currentValue);
				if (ImGui::Selectable(ToString(item), isSelected))
				{
					currentValue = item;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	void DragFloat3UNorm(const char* label, Float3& value)
	{
		ImGui::PushID(label);
		ImGui::PushItemWidth(50);
		ImGui::Text(label);
		ImGui::SameLine();
		ImGui::DragFloat("x", &value.x, 0.01f, 0.0f, 1.0f);
		ImGui::SameLine();
		ImGui::DragFloat("y", &value.y, 0.01f, 0.0f, 1.0f);
		ImGui::SameLine();
		ImGui::DragFloat("z", &value.z, 0.01f, 0.0f, 1.0f);
		ImGui::PopItemWidth();
		ImGui::PopID();
	}

	void DragFloat3(const char* label, Float3& value)
	{
		ImGui::PushID(label);
		ImGui::PushItemWidth(50);
		ImGui::Text(label);
		ImGui::SameLine();
		ImGui::DragFloat("x", &value.x);
		ImGui::SameLine();
		ImGui::DragFloat("y", &value.y);
		ImGui::SameLine();
		ImGui::DragFloat("z", &value.z);
		ImGui::PopItemWidth();
		ImGui::PopID();
	}

	class PBRSettingsGUI : public GUIElement
	{
	public:
		PBRSettingsGUI() : GUIElement("PBR settings", GUIFlags::None) {}
		void Update(float dt) override {}

		void Render(GraphicsContext& context) override
		{
			EnumSelect("BRDF Function", PBRCfg.BRDF_Function);
			DragFloat3UNorm("Subsurface albedo", PBRCfg.SubsurfaceAlbedo);

			ImGui::Separator();

			EnumSelect("F0 Calculation", PBRCfg.F0Calculaton);

			switch (PBRCfg.F0Calculaton)
			{
			case F0Calculation::Direct:
				DragFloat3("F0", PBRCfg.F0);
				DragFloat3("F90", PBRCfg.F90);
				ImGui::DragFloat("P", &PBRCfg.P, 0.01f);
				break;
			case F0Calculation::RefractionEstimate:
				ImGui::DragFloat("n1", &PBRCfg.n1, 0.01f);
				ImGui::DragFloat("n2", &PBRCfg.n2, 0.01f);

				const float f0Sqrt = (PBRCfg.n1 - PBRCfg.n2) / (PBRCfg.n1 + PBRCfg.n2);
				PBRCfg.F0 = Float3(f0Sqrt * f0Sqrt);
				PBRCfg.F90 = Float3(1.0f);
				PBRCfg.P = 0.2f;
				break;
			}

			EnumSelect("Fresnel reflectance", PBRCfg.FresnelReflectance);
		}
	};

	class LightSettingsGUI : public GUIElement
	{
	public:
		LightSettingsGUI() : GUIElement("Light settings", GUIFlags::None) {}
		void Update(float dt) override {}

		void Render(GraphicsContext& context) override
		{
			EnumSelect("Light type", PBRCfg.Illumination_Type);
			switch (PBRCfg.Illumination_Type)
			{
			case Illumination::None:
				DragFloat3("Light direction", PBRCfg.DirectionalLight);
				PBRCfg.DirectionalLight = PBRCfg.DirectionalLight.Normalize();
				break;
			case Illumination::Directional:
				DragFloat3("Light direction", PBRCfg.DirectionalLight);
				PBRCfg.DirectionalLight = PBRCfg.DirectionalLight.Normalize();
				break;
			case Illumination::Point:
				DragFloat3("Light position", PBRCfg.PointLight);
				break;
			}

			if (PBRCfg.Illumination_Type != Illumination::None)
			{
				DragFloat3("Light color", PBRCfg.LightColor);
			}
		}
	};

	class SceneSettingsGUI : public GUIElement
	{
	public:
		SceneSettingsGUI() : GUIElement("Scene settings", GUIFlags::None) {}
		void Update(float dt) override {}

		void Render(GraphicsContext& context) override
		{
			ImGui::DragFloat("Model rotation speed", &PBRCfg.ModelRotationSpeed);
		}
	};

	void AddGUI(PBRApp* cloudsApp)
	{
		GUI* gui = GUI::Get();
		gui->PushMenu("PBR");
		gui->AddElement(new PBRSettingsGUI{});
		gui->AddElement(new LightSettingsGUI{});
		gui->AddElement(new SceneSettingsGUI{});
		gui->PopMenu();
	}

	void RemoveGUI()
	{
		GUI::Get()->RemoveMenu("PBR");
	}
}