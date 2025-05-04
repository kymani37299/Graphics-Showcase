#include "MeshletsAppGUI.h"

#include <Engine/Render/Device.h>
#include <Engine/Gui/GUI.h>
#include <Engine/Gui/ImGui_Core.h>

#include "Grass/Settings.h"

namespace MeshletsAppGUI
{	void AddGUI()
	{
		GUI* gui = GUI::Get();
		gui->PushMenu("Meshlets");
		gui->PopMenu();
	}

	void RemoveGUI()
	{
		GUI::Get()->RemoveMenu("Meshlets");
	}
}