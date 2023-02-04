#include "GrassAppGUI.h"

#include <Engine/Render/Device.h>
#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui_Core.h>

#include "Grass/Settings.h"

namespace GrassAppGUI
{
	Requests GUIRequests;

	class GrassSettingsGUI : public GUIElement
	{
	public:
		GrassSettingsGUI(): GUIElement("Grass settings", GUIFlags::None) {}

		void Update(float dt) override {}

		void Render() override 
		{
			ImGui::PushItemWidth(100);
			ImGui::DragFloat("Tip Range", GrassSettings.TipRange, 0.1f);
			ImGui::DragFloat("Ambient occlusion Range", GrassSettings.AmbientOcclusionRange, 0.1f);
			ImGui::ColorEdit("Tip", GrassSettings.TipColor);
			ImGui::ColorEdit("Top", GrassSettings.TopColor);
			ImGui::ColorEdit("Bottom", GrassSettings.BottomColor);
			ImGui::ColorEdit("Ambient occlusion", GrassSettings.AmbientOcclusionColor);
			ImGui::PopItemWidth();
		}
	};

	class GrassPerfSettingsGUI : public GUIElement
	{
	public:
		GrassPerfSettingsGUI() : GUIElement("Grass performance settings", GUIFlags::None) {}

		void Update(float dt) override {}

		void Render() override
		{
			ImGui::PushItemWidth(100);
			ImGui::DragFloat("Lowpoly distance treshold", &GrassPerfSettings.LowpolyTreshold, 0.1f);
			ImGui::DragFloat("Instance reduction factor", &GrassPerfSettings.InstanceReductionFactor, 0.1f);
			ImGui::DragUint("Minimum instances per patch", GrassPerfSettings.MinInstancesPerPatch);
			ImGui::PopItemWidth();
		}
	};

	class GrassGenerationGUI : public GUIElement
	{
	public:
		GrassGenerationGUI(): GUIElement("Grass generation", GUIFlags::None) {}

		void Update(float dt) override {}

		void Render() override
		{
			if (ImGui::Button("Regenerate grass")) GUIRequests.RegenerateGrass = true;
			ImGui::PushItemWidth(100);
			ImGui::DragUint("Number of instances: ", GrassGenConfig.NumInstances, 1000);
			ImGui::DragFloat("Height range", GrassGenConfig.HeightRange, 0.1f);
			ImGui::DragFloat("Position", GrassGenConfig.PlanePosition);
			ImGui::DragFloat("Scale", GrassGenConfig.PlaneScale);
			ImGui::PopItemWidth();
		}
	};

	class ShowWindTextureGUIButton : public GUIElement
	{
	public:
		ShowWindTextureGUIButton(): GUIElement("Show wind texture", GUIFlags::OnlyButton) {}

		void OnMenuButtonPress()
		{
			GUIRequests.ToggleWindTexture = true;
		}
	};

	void AddGUI()
	{
		GUI* gui = GUI::Get();
		gui->PushMenu("Grass");
		gui->AddElement(new GrassSettingsGUI{});
		gui->AddElement(new GrassPerfSettingsGUI{});
		gui->AddElement(new GrassGenerationGUI{});
		gui->AddElement(new ShowWindTextureGUIButton{});
		gui->PopMenu();
	}

	void RemoveGUI()
	{
		GUI::Get()->RemoveMenu("Grass");
	}
}