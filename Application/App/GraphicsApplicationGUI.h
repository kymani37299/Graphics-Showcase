#pragma once

#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui_Core.h>

#include "App/GraphicsApplication.h"
#include "Common/Camera.h"

class GraphicsApplicationGUI : public GUIElement
{
public:
	GraphicsApplicationGUI(GraphicsApplication* application) :
		GUIElement("Samples"),
		m_Application(application)
	{ }

	void Update(float dt) override {}

protected:

	void Render(GraphicsContext& context) override
	{
		ImGui::Text("Active: %s", m_Application->GetActiveSampleName().c_str());

		if (ImGui::Button("Previous"))
		{
			m_Application->PreviousSample();
		}

		ImGui::SameLine();

		if (ImGui::Button("Next"))
		{
			m_Application->NextSample();
		}
	}

private:
	GraphicsApplication* m_Application;
};

class ControlsGUI : public GUIElement
{
public:
	ControlsGUI() :
		GUIElement("Controls", GUIFlags::ShowOnStart)
	{ }

	void Update(float dt) override {}

protected:

	void Render(GraphicsContext& context) override
	{
		ImGui::Text("WASD - Movement");
		ImGui::Text("F    - Toggle mouse");
		ImGui::Text("R    - Reload shaders");
		ImGui::Text("G    - Togggle gui");
		ImGui::Text("Z    - Previous sample");
		ImGui::Text("X    - Next sample");

		ImGui::Separator();

		ImGui::DragFloat("Camera speed", &Camera::s_CameraSpeed, 0.1f);
	}
};

class RenderStatsGUI : public GUIElement
{
public:
	RenderStatsGUI() : GUIElement("Render stats", GUIFlags::ShowOnStart) {}

	void Update(float dt) override
	{
		static constexpr float UpdateInterval = 200.0f; // In ms
		static float lastUpdate = 0.0f;

		lastUpdate += dt;
		DTHistory.push_back(dt);
		if (lastUpdate > UpdateInterval)
		{
			size_t count = DTHistory.size();
			float sum = 0.0f;
			for (float _dt : DTHistory)
			{
				sum += _dt;
			}

			lastUpdate -= UpdateInterval;
			m_CurrentDT = sum / count;

			DTHistory.clear();
		}
	}

	void Render(GraphicsContext& context) override
	{
		ImGui::Text("Frame: %.2f ms", m_CurrentDT);
		ImGui::Text("FPS:   %u", static_cast<uint32_t>(1000.0f / m_CurrentDT));
	}

private:
	std::vector<float> DTHistory;
	float m_CurrentDT = 0.0f;
};