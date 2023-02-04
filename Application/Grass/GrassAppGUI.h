#pragma once

namespace GrassAppGUI
{
	struct Requests
	{
		bool RegenerateGrass = false;
		bool ToggleWindTexture = false;
	};

	void AddGUI();
	void RemoveGUI();

	extern Requests GUIRequests;
}