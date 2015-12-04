
// Estimate direct illumination from a lambertian emitter (including shadows)
vec3 EstimateDirectLight(vec3 _pos, vec3 _normal, vec3 _lightPos, vec3 _lightNormal, vec3 _intensity, vec3 _viewDir, MaterialData _materialData)
{
	Ray lightRay;
	// Direction to light & distance.
	lightRay.Direction = _pos - _lightPos;
	float lightDistSq = dot(lightRay.Direction, lightRay.Direction) + DIVISOR_EPSILON;
	float lightDist = sqrt(lightDistSq);
	lightRay.Direction /= lightDist;

	// Facing the light?
	float lightSampleIntensityFactor = saturate(dot(_lightNormal, lightRay.Direction));
	float surfaceCos = saturate(-dot(lightRay.Direction, _normal));
	if(lightSampleIntensityFactor > 0.0 && surfaceCos > 0.0)
	{
		lightRay.Origin = RAY_HIT_EPSILON * lightRay.Direction + _lightPos;

		if(!TraceRayAnyHit(lightRay, lightDist - RAY_HIT_EPSILON * 2))
		{
			// Hemispherical lambert emitter. First compensate the cosine factor
			vec3 lightSampleIntensity = _intensity.xyz * lightSampleIntensityFactor; // lightIntensity = lightIntensity in normal direction (often called I0) -> seen area is smaller to the border
			vec3 bsdf = BSDF(_viewDir, -lightRay.Direction, _materialData, _normal);

			vec3 irradiance = (surfaceCos / lightDistSq) * lightSampleIntensity; // Use saturate, otherwise light from behind may come in because of shading normals!
			return irradiance * bsdf;
		}
	}
	
	return vec3(0.0);
}