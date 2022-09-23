#include "GrassAppGUI.h"

#include <Engine/Render/Device.h>
#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui.h>

#include "Grass/Settings.h"
#include "Grass/GrassApp.h"

namespace GrassAppGUI
{
	class GrassSettingsGUI : public GUIElement
	{
	public:
		GrassSettingsGUI(): GUIElement("Grass settings", false) {}

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

	class GrassGenerationGUI : public GUIElement
	{
	public:
		GrassGenerationGUI(GrassApp* app): GUIElement("Grass generation", false), m_App(app) {}

		void Update(float dt) override {}

		void Render() override
		{
			if (ImGui::Button("Regenerate grass")) m_App->RegenerateGrass(Device::Get()->GetContext());
			ImGui::PushItemWidth(100);
			ImGui::DragUint("Number of instances: ", GrassGenConfig.NumInstances, 1000);
			ImGui::DragFloat("Height range", GrassGenConfig.HeightRange, 0.1f);
			ImGui::DragFloat("Position", GrassGenConfig.PlanePosition);
			ImGui::DragFloat("Scale", GrassGenConfig.PlaneScale);
			ImGui::PopItemWidth();
		}

	private:
		GrassApp* m_App;
	};

	class GrassStatsGUI : public GUIElement
	{
	public:
		GrassStatsGUI() : GUIElement("Grass stats", false) {}

		void Update(float dt) override {}

		void Render() override
		{
			ImGui::Text("Patches:   %u / %u", GrassStats.PatchesDrawn, GrassStats.TotalPatches);
			ImGui::Text("Instances: %u / %u", GrassStats.InstancesDrawn, GrassStats.TotalInstances);

		}
	};

	void AddGUI(GrassApp* app)
	{
		GUI* gui = GUI::Get();
		gui->PushMenu("Grass");
		gui->AddElement(new GrassSettingsGUI{});
		gui->AddElement(new GrassGenerationGUI{app});
		gui->AddElement(new GrassStatsGUI{});
		gui->PopMenu();
	}

	void RemoveGUI()
	{
		GUI::Get()->RemoveMenu("Grass");
	}
}