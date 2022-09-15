#include "GraphicsApplication.h"

#include <Engine/Gui/GUI.h>
#include <Engine/Gui/Imgui/imgui.h>
#include <Engine/System/Window.h>

class GraphicsApplicationGUI : public GUIElement
{
public:
	GraphicsApplicationGUI(GraphicsApplication* application):
		GUIElement("Active application", true),
		m_Application(application)
	{ }
	
	void Update(float dt) override {}

protected:

	void Render() override
	{
		ImGui::Text("Active: %u", m_Application->m_ActiveApplication);

		if (ImGui::Button("Next"))
		{
			m_Application->m_ActiveApplication = (m_Application->m_ActiveApplication + 1) % m_Application->m_Applications.size();
		}

		if (ImGui::Button("Previous"))
		{
			m_Application->m_ActiveApplication = (m_Application->m_ActiveApplication - 1) % m_Application->m_Applications.size();
		}

	}

private:
	GraphicsApplication* m_Application;
};

void GraphicsApplication::OnInit_Internal()
{
	Window::Get()->ShowCursor(true);
	GUI::Get()->AddElement(new GraphicsApplicationGUI(this));
}
