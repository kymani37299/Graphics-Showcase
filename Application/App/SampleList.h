#include <vector>
#include <functional>

#include "Animation/AnimationApp.h"
#include "Clouds/CloudsApp.h"
#include "Grass/GrassApp.h"
#include "VolumetricLights/VolumetricLightsApp.h"
#include "PBR/PBRApp.h"
#include "Meshlets/MeshletsApp.h"

struct SampleEntry
{
	const char* Name;
	std::function<Application* ()> CreateFunction;
};

inline const std::vector<SampleEntry> s_SampleList = {
	SampleEntry{ "PBR Sample",   []() -> Application* { return new PBRApp{}; } },
	SampleEntry{ "Grass Sample", []() -> Application* { return new GrassApp{}; } },
	SampleEntry{ "Clouds Sample",[]() -> Application* { return new CloudsApp{}; } },
	//SampleEntry{ "Animation Sample",   []() -> Application* { return new AnimationApp{}; } },
	//SampleEntry{ "Volumetric Lights Sample",   []() -> Application* { return new VolumetricLightsApp{}; } },
	//SampleEntry{ "Meshlets Sample",   []() -> Application* { return new MeshletsApp{}; } },
};