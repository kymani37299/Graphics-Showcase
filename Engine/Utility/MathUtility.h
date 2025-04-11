#pragma once

#include "Utility/DataTypes.h"

namespace MathUtility
{
	// ceil(a/b)
	template<typename T>
	inline T CeilDiv(T a, T b) { return (a + b - 1) / b; }

	template<typename T>
	inline T Align(T a, T alignment) { return a + alignment - 1 - (a + alignment - 1) % alignment; }

	template<typename T>
	T Lerp(const T& a, const T& b, float t) { return a + t * (b - a); }

	template<>
	inline Quaternion Lerp(const Quaternion& qa, const Quaternion& qb, float t)
	{
		Float4 a{ qa.x, qa.y, qa.z, qa.w };
		Float4 b{ qb.x, qb.y, qb.z, qb.w };

		float cosTheta = a.Dot(b);

		if (cosTheta < 0.0f)
		{
			b *= -1.0f;
			cosTheta *= -1.0f;
		}

		// Early exit: If a and b too close we can just use linear interpolation
		constexpr float Epsilon = 0.0005f;
		if (cosTheta > 1.0f - Epsilon) return Lerp(a, b, t);

		const float theta = acos(cosTheta);
		const float sinTheta = sin(theta);

		const float sigma = t * theta;
		const float cosSigma = cos(sigma);
		const float sinSigma = sin(sigma);

		const float factorA = cosSigma - (cosTheta * sinSigma) / sinTheta;
		const float factorB = sinSigma / sinTheta;

		return factorA * a + factorB * b;
	}

	inline Float3 QuaternionToEulerAngles(const Float4& quat)
	{
		Float3 result; // (Pitch, Yaw, Roll)
		result.x = atan2(2.0f * quat.x * quat.w - 2.0f * quat.y * quat.z, 1.0f - 2.0f * quat.x * quat.x - 2.0f * quat.z * quat.z);
		result.y = atan2(2.0f * quat.y * quat.w - 2.0f * quat.x * quat.z, 1.0f - 2.0f * quat.y * quat.y - 2.0f * quat.z * quat.z);
		result.z = asin(2.0f * quat.x * quat.y + 2.0f * quat.z * quat.w);
		return result;
	}

}