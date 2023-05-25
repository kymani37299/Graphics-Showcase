#include "Input.h"

#include "System/Window.h"

namespace Input
{
	// Return true every frame when key is pressed
	bool IsKeyPressed(unsigned int key)
	{
		return WindowInput::IsKeyPressed(key);
	}

	// Return true only first frame when the key is pressed
	bool IsKeyJustPressed(unsigned int key)
	{
		return WindowInput::IsKeyJustPressed(key);
	}

	// [0,1] range if the mouse is in the window
	Float2 GetMousePos()
	{
		return WindowInput::GetMousePos();
	}

	// GetMousePos(CurrentFrame) - GetMousePos(LastFrame) 
	Float2 GetMouseDelta()
	{
		return WindowInput::GetMouseDelta();
	}
}