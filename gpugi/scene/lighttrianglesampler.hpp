#pragma once

#include <memory>
#include <ei/matrix.hpp>
#include <cinttypes>

class Scene;

class LightTriangleSampler
{
public:
	LightTriangleSampler();

	void SetScene(std::shared_ptr<const Scene> _scene);

	/// Data representation of a light sample.
	struct LightSample
	{
		ei::Vec3 position;
		std::int16_t normalThetaCos;	// normal.z
		std::int16_t normalPhi;			// atan(normal.xy)
		ei::Vec3 luminance;
		std::int32_t previousLightSample; ///< Will be set to -1 for all "initial" samples.
	};

	/// \param positionBias
	///		Moves the sample a bit along the triangle normal to avoid precision issues.
	void GenerateRandomSamples(LightSample* _destinationBuffer, unsigned int _numSamples, float _positionBias = 0.02f);

private:
	std::shared_ptr<const Scene> m_scene;
	std::uint64_t m_randomSeed;
};