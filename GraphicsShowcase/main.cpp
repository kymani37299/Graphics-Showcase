#include <Windows.h>
#include <iostream>
#include <time.h>

#include <Engine/Core/Engine.h>
#include <Engine/System/VSConsoleRedirect.h>
#include <Engine/System/ApplicationConfiguration.h>
#include <Engine/Utility/StringUtility.h>

#include "App/GraphicsApplication.h"

void ReadCommandArguments(const std::string& arguments)
{
	const auto argumentList = StringUtility::Split(arguments, " ");
	for (const auto argument : argumentList)
	{
		std::string finalArgument = StringUtility::ToUpper(argument);
		StringUtility::ReplaceAll(finalArgument, "-", "");
		StringUtility::ReplaceAll(finalArgument, " ", "");
		AppConfig.Settings.insert(finalArgument);
	}
}

class EmptyApp : public Application
{

};

GraphicsApplication* CreateGraphicsApplication()
{
	std::vector<Application*> applications{};
	applications.push_back(new EmptyApp{});
	applications.push_back(new EmptyApp{});

	return new GraphicsApplication{ applications };
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdParams, int showFlags)
{
	AppConfig.AppHandle = instance;
	AppConfig.WindowTitle = "Nature";
	AppConfig.WindowWidth = 1024;
	AppConfig.WindowHeight = 768;
	ReadCommandArguments(std::string(cmdParams));

	RedirectToVSConsoleScoped _vsConsoleRedirect;

	srand((uint32_t)time(0));

	Engine e{ CreateGraphicsApplication() };
	e.Run();
}