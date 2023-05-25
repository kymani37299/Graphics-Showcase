#pragma once

#include "Common.h"

namespace Input
{
	// Return true every frame when key is pressed
	bool IsKeyPressed(unsigned int key);

	// Return true only first frame when the key is pressed
	bool IsKeyJustPressed(unsigned int key);

	// [0,1] range if the mouse is in the window
	Float2 GetMousePos();

	// GetMousePos(CurrentFrame) - GetMousePos(LastFrame) 
	Float2 GetMouseDelta();
}