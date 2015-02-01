#pragma once

#include <memory>
#include <ei/vector.hpp>
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
		float normalPhi;			// atan2(normal.y, normal.x)
		ei::Vec3 intensity;
		float normalThetaCos;	// normal.z
	};

	/// \param positionBias
	///		Moves the sample a bit along the triangle normal to avoid precision issues.
	void GenerateRandomSamples(LightSample* _destinationBuffer, unsigned int _numSamples, float _positionBias = 0.1f);

private:
	std::shared_ptr<const Scene> m_scene;
	std::uint64_t m_randomSeed;
};