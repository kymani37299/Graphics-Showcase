#pragma once

#include <Engine/Common.h>

enum class BRDF
{
	None = 0,
	Lambert,
	Count,
};

enum class Illumination
{
	None,
	Directional,
	Point,
	Count,
};

enum class F0Calculation
{
	Direct,
	RefractionEstimate,
	Count,
};

enum class FresnelReflectance
{
	None,
	Shlick,
	Count,
};

struct PBRConfig
{
	BRDF BRDF_Function = BRDF::None;
	Float3 SubsurfaceAlbedo = Float3(0.5f, 0.5f, 0.5f);

	Illumination Illumination_Type = Illumination::None;
	Float3 DirectionalLight = Float3(0.1f, 0.6f, 0.8f);
	Float3 PointLight = Float3(1.0f);
	Float3 LightColor = Float3(10.0f);

	F0Calculation F0Calculaton;
	Float3 F0 = Float3(0.04f);
	Float3 F90 = Float3(1.0f);
	float P = 0.2f;
	float n1 = 1.0f;
	float n2 = 1.33f;

	FresnelReflectance FresnelReflectance;

	float ModelRotationSpeed = 1.0f;
};

extern PBRConfig PBRCfg;

#define ENUM_TO_STRING(type, value) case type::value: return #value
#define DEFAULT_ENUM_TO_STRING default: return "Unknown"

const char* ToString(BRDF brdf)
{
	switch (brdf)
	{
		ENUM_TO_STRING(BRDF, None);
		ENUM_TO_STRING(BRDF, Lambert);
		DEFAULT_ENUM_TO_STRING;
	}
}

const char* ToString(Illumination illumination)
{
	switch (illumination)
	{
		ENUM_TO_STRING(Illumination, None);
		ENUM_TO_STRING(Illumination, Directional);
		ENUM_TO_STRING(Illumination, Point);
		DEFAULT_ENUM_TO_STRING;
	}
}

const char* ToString(F0Calculation f0calc)
{
	switch (f0calc)
	{
		ENUM_TO_STRING(F0Calculation, Direct);
		ENUM_TO_STRING(F0Calculation, RefractionEstimate);
		DEFAULT_ENUM_TO_STRING;
	}
}

const char* ToString(FresnelReflectance reflectance)
{
	switch (reflectance)
	{
		ENUM_TO_STRING(FresnelReflectance, None);
		ENUM_TO_STRING(FresnelReflectance, Shlick);
		DEFAULT_ENUM_TO_STRING;
	}
}

#undef DEFAULT_ENUM_TO_STRING
#undef ENUM_TO_STRING
