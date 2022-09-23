#include <Engine/Core/EngineMain.h>

#include "App/GraphicsApplication.h"

Application* Main(ApplicationConfiguration& appConfig)
{
	AppConfig.WindowTitle = "Graphics showcase";
	AppConfig.WindowWidth = 1024;
	AppConfig.WindowHeight = 768;

	return new GraphicsApplication();
}