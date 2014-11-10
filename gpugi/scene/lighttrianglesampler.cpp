﻿#include "lighttrianglesampler.hpp"
#include "scene.hpp"
#include "../utilities/random.hpp"
#include "../utilities/assert.hpp"

LightTriangleSampler::LightTriangleSampler() : m_randomSeed(123)
{

}

void LightTriangleSampler::SetScene(std::shared_ptr<const Scene> _scene)
{
	m_scene = _scene;
}

void LightTriangleSampler::GenerateRandomSamples(LightSample* _destinationBuffer, unsigned int _numSamples, float _positionBias)
{
	float sampleWeight = 1.0f / _numSamples;

	for (unsigned int sampleIdx = 0; sampleIdx < _numSamples; ++sampleIdx, ++_destinationBuffer)
	{
		float randomTriangle = 0;
		m_randomSeed = Xorshift(m_randomSeed, randomTriangle);
		const float* lightTriangleSummedArea = m_scene->GetLightSummedArea();

		// Search triangle.
		unsigned int triangleIdx = 0;
		for (; triangleIdx < m_scene->GetNumLightTriangles(); ++lightTriangleSummedArea, ++triangleIdx)
		{
			if (randomTriangle < *lightTriangleSummedArea)
				break;
		}
		Assert(triangleIdx < m_scene->GetNumLightTriangles(), "Impossible triangle index. Error in random number generator or light summed area table.");

		// Compute random barycentric coordinates.
		float alpha = 0;
		m_randomSeed = Xorshift(m_randomSeed, alpha);
		float beta = 0;
		m_randomSeed = Xorshift(m_randomSeed, beta);
		beta *= 1.0f - alpha;
		float gamma = 1.0f - (alpha + beta);

		// Compute light sample and write to buffer.
		const Scene::LightTriangle* lightTriangle = m_scene->GetLightTriangles() + triangleIdx;
		_destinationBuffer->position = lightTriangle->triangle.v0 * alpha + 
										lightTriangle->triangle.v1 * beta +
										lightTriangle->triangle.v2 * gamma;
		ei::Vec3 normal = ei::normalize(ei::cross(lightTriangle->triangle.v0 - lightTriangle->triangle.v1, lightTriangle->triangle.v0 - lightTriangle->triangle.v2));
		_destinationBuffer->position += normal * _positionBias;// Move a bit along the normal to avoid intersection precision issues.

		_destinationBuffer->normalThetaCos = static_cast<std::int16_t>(normal.z * std::numeric_limits<std::int16_t>().max());
		_destinationBuffer->normalPhi = static_cast<std::int16_t>(atan2(normal.x, normal.y) * std::numeric_limits<std::int16_t>().max());
		_destinationBuffer->luminance = lightTriangle->luminance * sampleWeight; // Area factor is already contained, since it determines the sample probability.
		_destinationBuffer->previousLightSample = -1;
	}
}