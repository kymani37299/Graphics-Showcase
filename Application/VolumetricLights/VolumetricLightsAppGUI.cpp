#include "VolumetricLightsAppGUI.h"

#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui_Core.h>

#include "VolumetricLights/Settings.h"

namespace VolumetricLightsAppGUI
{
	void AddGUI()
	{
		GUI* gui = GUI::Get();
		gui->PushMenu("Volumetric Lights");
		gui->PopMenu();
	}

	void RemoveGUI()
	{
		GUI::Get()->RemoveMenu("Volumetric Lights");
	}
}