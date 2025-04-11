#include "AnimationAppGUI.h"

#include <vector>

#include <Engine/Loading/ModelLoading.h>
#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui_Core.h>

#include "Animation/AnimationApp.h"

namespace AnimationAppGUI
{
	class MorphsGUI : public GUIElement
	{
	public:
		MorphsGUI(AnimationApp* app) :
			GUIElement("Morphs"),
			m_Application(app)
		{}

		void Update(float dt) override {}

		void Render(GraphicsContext& context) override
		{
			ImGui::Checkbox("Weight animation", &m_Application->m_EnableWeightAnimation);

			uint32_t objectIndex = 0;
			for (ModelLoading::SceneObject& object : m_Application->m_Scene.Objects)
			{
				if (object.MorphTargets.empty()) continue;

				ImGui::PushID(objectIndex++);
				ImGui::Text("%u", objectIndex);
				uint32_t targetIndex = 0;
				for (ModelLoading::MorphTarget& target : object.MorphTargets)
				{
					const std::string label = "W" + std::to_string(targetIndex);
					ImGui::PushID(targetIndex++);
					ImGui::DragFloat(label.c_str(), &target.Weight, 0.1f);
					ImGui::PopID();
				}
				ImGui::Separator();
				ImGui::PopID();
			}
		}

	private:
		AnimationApp* m_Application;
	};

	void AddGUI(AnimationApp* app)
	{
		GUI* gui = GUI::Get();
		gui->PushMenu("Animations");
		gui->AddElement(new MorphsGUI(app));
		gui->PopMenu();
	}

	void RemoveGUI()
	{
		GUI::Get()->RemoveMenu("Animations");
	}
}