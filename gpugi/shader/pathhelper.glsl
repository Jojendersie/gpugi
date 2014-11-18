bool ContinuePath(inout vec3 rayColor, inout Ray ray, inout uint randomSeed, 
				  vec3 hitNormal, int triangleIdx, MaterialTextureData materialTexData)
{
	vec3 weight;
	ray.Direction = SampleBRDF(ray.Direction, triangleIdx, materialTexData, randomSeed, hitNormal, weight);
	ray.Origin = ray.Origin + RAY_HIT_EPSILON * sign(dot(ray.Direction, hitNormal)) * hitNormal;	// Go epsilon in sampling direction (refraction/reflection different)
#ifdef RUSSIAN_ROULETTE
	float hitLuminance = GetLuminance(weight);
	rayColor *= weight / hitLuminance; // Only change in spectrum, no energy loss.
	return Random(randomSeed) <= hitLuminance;
#else
	rayColor *= weight; // Absorption, not via Russion Roulette, but by color multiplication.
	return true;
#endif
}