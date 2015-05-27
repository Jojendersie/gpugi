#include "lightsampler.hpp"
#include "scene.hpp"
#include "../utilities/random.hpp"
#include "../utilities/assert.hpp"

#include <algorithm>

LightSampler::LightSampler() : m_randomSeed(123)
{

}

void LightSampler::SetScene(std::shared_ptr<const Scene> _scene)
{
	m_scene = _scene;
}

void LightSampler::GenerateRandomSamples(LightSample* _destinationBuffer, unsigned int _numSamples, float _positionBias)
{
	float pointLightPercentage = m_scene->GetTotalPointLightFlux() / (m_scene->GetTotalAreaLightFlux() + m_scene->GetTotalPointLightFlux());
	unsigned numPointSamples = unsigned(pointLightPercentage * _numSamples);
	unsigned numAreaSamples = _numSamples - numPointSamples;
	float sampleWeight = m_scene->GetLightAreaSum() / numAreaSamples;

	//if(numPointSamples < m_scene->GetPointLights().size())
	//	LOG_ERROR("The number of initial light samples is too small for the number of point lights!");

	for (unsigned int sampleIdx = 0; sampleIdx < numAreaSamples; ++sampleIdx, ++_destinationBuffer)
	{
		float randomTriangle = XorshiftF(m_randomSeed);

		// Search triangle.
		const float* lightTriangleSummedArea = std::lower_bound(m_scene->GetLightSummedAreaTable(), m_scene->GetLightSummedAreaTable() + m_scene->GetNumLightTriangles(), randomTriangle);
		unsigned int triangleIdx = static_cast<unsigned int>(lightTriangleSummedArea - m_scene->GetLightSummedAreaTable());
		Assert(triangleIdx < m_scene->GetNumLightTriangles(), "Impossible triangle index. Error in random number generator or light summed area table.");

		//float triangleArea = *lightTriangleSummedArea;
		//if(triangleIdx > 0) triangleArea -= lightTriangleSummedArea[-1];

		// Compute random barycentric coordinates.
		// OFCD02shapedistributions sec 4.2
		float xi0 = XorshiftF(m_randomSeed);
		float xi1 = XorshiftF(m_randomSeed);
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

	// Importance sampling for point light sources
	for (unsigned int sampleIdx = 0; sampleIdx < numPointSamples; ++sampleIdx, ++_destinationBuffer)
	{
		float randomPointLight = XorshiftF(m_randomSeed);

		// Search light source.
		const float* lightSummedFlux = std::lower_bound(m_scene->GetPointLightSummedFluxTable(), m_scene->GetPointLightSummedFluxTable() + m_scene->GetPointLights().size(), randomPointLight);
		unsigned int lightIdx = static_cast<unsigned int>(lightSummedFlux - m_scene->GetPointLightSummedFluxTable());
		Assert(lightIdx < m_scene->GetPointLights().size(), "Impossible light index. Error in random number generator or light summed flux table.");
		sampleWeight = *lightSummedFlux;
		if(lightIdx > 0) sampleWeight -= m_scene->GetPointLightSummedFluxTable()[lightIdx-1];
		sampleWeight = 1.0f / (numPointSamples * sampleWeight);

		// Compute light sample and write to buffer.
		_destinationBuffer->position = m_scene->GetPointLights()[lightIdx].position;
		// Random normal
		_destinationBuffer->normalPhi = 2 * ei::PI * XorshiftF(m_randomSeed);
		_destinationBuffer->normalThetaCos = XorshiftF(m_randomSeed) * 2.0f - 1.0f;
		ei::Vec3 normal = ei::cartesianCoords(ei::Vec3(1.0f, _destinationBuffer->normalThetaCos, _destinationBuffer->normalPhi));
		// Move a bit along the normal to avoid intersection precision issues.
		_destinationBuffer->position += normal * (_positionBias);
		// A light sample is always a hemispherical Lambert emitter -> factor 4 too dark
		_destinationBuffer->intensity = m_scene->GetPointLights()[lightIdx].intensity * sampleWeight * 4.0f;
	}
}