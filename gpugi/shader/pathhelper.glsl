bool ContinuePath(inout vec3 rayColor, inout Ray ray, inout uint randomSeed, out float connectionPropability,
				  vec3 hitNormal, int triangleIdx, MaterialTextureData materialTexData)
{
	vec3 weight;
	ray.Direction = SampleBSDF(ray.Direction, triangleIdx, materialTexData, randomSeed, hitNormal, weight);
	ray.Origin += ray.Direction * RAY_HIT_EPSILON;
		
#ifdef RUSSIAN_ROULETTE
	connectionPropability = GetLuminance(weight);
	rayColor *= weight / connectionPropability; // Only change in spectrum, no energy loss.
	return Random(randomSeed) <= connectionPropability;
#else
	rayColor *= weight; // Absorption, not via Russion Roulette, but by color multiplication.
	return true;
#endif
}

bool ContinuePath(inout vec3 rayColor, inout Ray ray, inout uint randomSeed,
				  vec3 hitNormal, int triangleIdx, MaterialTextureData materialTexData)
{
	float connectionPropability;
	return ContinuePath(rayColor, ray, randomSeed, connectionPropability, hitNormal, triangleIdx, materialTexData);
}