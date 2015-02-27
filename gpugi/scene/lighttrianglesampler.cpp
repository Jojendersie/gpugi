﻿#include "lighttrianglesampler.hpp"
#include "scene.hpp"
#include "../utilities/random.hpp"
#include "../utilities/assert.hpp"

#include <algorithm>

LightTriangleSampler::LightTriangleSampler() : m_randomSeed(123)
{

}

void LightTriangleSampler::SetScene(std::shared_ptr<const Scene> _scene)
{
	m_scene = _scene;
}

void LightTriangleSampler::GenerateRandomSamples(LightSample* _destinationBuffer, unsigned int _numSamples, float _positionBias)
{
	float sampleWeight = m_scene->GetLightAreaSum() / _numSamples;

	for (unsigned int sampleIdx = 0; sampleIdx < _numSamples; ++sampleIdx, ++_destinationBuffer)
	{
		float randomTriangle = 0;
		m_randomSeed = Xorshift(m_randomSeed, randomTriangle);

		// Search triangle.
		const float* lightTriangleSummedArea = std::lower_bound(m_scene->GetLightSummedAreaTable(), m_scene->GetLightSummedAreaTable() + m_scene->GetNumLightTriangles(), randomTriangle);
		unsigned int triangleIdx = static_cast<unsigned int>(lightTriangleSummedArea - m_scene->GetLightSummedAreaTable());
		Assert(triangleIdx < m_scene->GetNumLightTriangles(), "Impossible triangle index. Error in random number generator or light summed area table.");

		//float triangleArea = *lightTriangleSummedArea;
		//if(triangleIdx > 0) triangleArea -= lightTriangleSummedArea[-1];

		// Compute random barycentric coordinates.
		// OFCD02shapedistributions sec 4.2
		float xi0, xi1;
		m_randomSeed = Xorshift(m_randomSeed, xi0);
		m_randomSeed = Xorshift(m_randomSeed, xi1);
		xi0 = sqrt(xi0);
		float alpha = (1.0f - xi0);
		float beta = xi0 * (1.0f - xi1);
		float gamma = xi0 * xi1;

		// Compute light sample and write to buffer.
		const Scene::LightTriangle* lightTriangle = m_scene->GetLightTriangles() + triangleIdx;
		_destinationBuffer->position = lightTriangle->triangle.v0 * alpha + 
										lightTriangle->triangle.v1 * beta +
										lightTriangle->triangle.v2 * gamma;
		ei::Vec3 normal = ei::normalize(ei::cross(lightTriangle->triangle.v0 - lightTriangle->triangle.v1, lightTriangle->triangle.v0 - lightTriangle->triangle.v2));
		// Move a bit along the normal to avoid intersection precision issues.
		_destinationBuffer->position += normal * (_positionBias); // * triangleArea

		_destinationBuffer->normalPhi = atan2(normal.y, normal.x);
		_destinationBuffer->intensity = lightTriangle->luminance * sampleWeight; // Area factor is already contained, since it determines the sample probability.
		_destinationBuffer->normalThetaCos = normal.z;
	}
}