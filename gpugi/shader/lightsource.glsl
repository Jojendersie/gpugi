// Find out what type of light source is given and estimate the reflected radiance
// for the given surface.
vec3 EstimateDirectLight(vec3 _pos, vec3 _normal, int _lightSampleIndex, vec3 _viewDir, MaterialData _materialData, bool _sampleAreaLights
#ifdef TRACERAY_IMPORTANCE_BREAK
	, in float _importanceThreshold
#endif
#ifdef CACHE_DIRECT_DIFFUSE
	, in int _diffuseCacheIndex
#endif
)
{
	vec4 lightSamplePos_Norm0 = texelFetch(InitialLightSampleBuffer, _lightSampleIndex * 2);
	vec4 lightIntensity_Norm1 = texelFetch(InitialLightSampleBuffer, _lightSampleIndex * 2 + 1);

	Ray lightRay;
	// Direction to light & distance.
	lightRay.Direction = _pos - lightSamplePos_Norm0.xyz;
	float lightDistSq = dot(lightRay.Direction, lightRay.Direction) + DIVISOR_EPSILON;
	float lightDist = sqrt(lightDistSq);
	lightRay.Direction /= lightDist;
	float surfaceCos = saturate(-dot(lightRay.Direction, _normal));

	if(lightIntensity_Norm1.w > 1.0) // Wrong normals encode omnidirectional point lights
	{
		// Omnidirectional (point) light
		// Facing the light?
		if(surfaceCos > 0.0)
		{
			lightRay.Origin = RAY_HIT_EPSILON * lightRay.Direction + lightSamplePos_Norm0.xyz;

		#ifdef CACHE_DIRECT_DIFFUSE
			vec3 specRadiance = vec3(0.0);
			vec4 diffuse_count = vec4(0.0);
		#endif
		#ifdef TRACERAY_IMPORTANCE_BREAK
			if(!TraceRayAnyHit(lightRay, lightDist - RAY_HIT_EPSILON * 2, _importanceThreshold))
		#else
			if(!TraceRayAnyHit(lightRay, lightDist - RAY_HIT_EPSILON * 2))
		#endif
			{
				vec3 irradiance = (surfaceCos / lightDistSq) * lightIntensity_Norm1.xyz;
			#ifdef CACHE_DIRECT_DIFFUSE
				float pdf;
				vec3 bsdfSpec = BSDFSpecular(_viewDir, -lightRay.Direction, _materialData, _normal, pdf);
				diffuse_count = vec4(irradiance * GetDiffuseReflectance(_materialData, surfaceCos) / PI, 1.0);
				specRadiance = irradiance * bsdfSpec;
			#else
				float pdf;
				vec3 bsdf = BSDF(_viewDir, -lightRay.Direction, _materialData, _normal, pdf);
				return irradiance * bsdf;
			#endif
			}
		#ifdef CACHE_DIRECT_DIFFUSE
			if(_diffuseCacheIndex != 0xFFFFFFFF)
			{
				diffuse_count += DiffuseLightCache[_diffuseCacheIndex];
				DiffuseLightCache[_diffuseCacheIndex] = diffuse_count;
			}
			return specRadiance + diffuse_count.xyz / (diffuse_count.w + 1e-10);
		#endif
		}
	} else if(_sampleAreaLights) {
		// Lambertian emitter (light emitting surface like area lights)
		vec3 lightNormal = UnpackNormal(vec2(lightSamplePos_Norm0.w, lightIntensity_Norm1.w));

		// Facing the light and light facing the surface?
		float lightSampleIntensityFactor = saturate(dot(lightNormal, lightRay.Direction));
		if(lightSampleIntensityFactor > 0.0 && surfaceCos > 0.0)
		{
			lightRay.Origin = RAY_HIT_EPSILON * lightRay.Direction + lightSamplePos_Norm0.xyz;

			#ifdef TRACERAY_IMPORTANCE_BREAK
			if(!TraceRayAnyHit(lightRay, lightDist - RAY_HIT_EPSILON * 2, _importanceThreshold))
			#else
			if(!TraceRayAnyHit(lightRay, lightDist - RAY_HIT_EPSILON * 2))
			#endif
			{
				// TODO: diffuse light cache
				float pdf;
				// Hemispherical lambert emitter. First compensate the cosine factor
				vec3 lightSampleIntensity = lightIntensity_Norm1.xyz * lightSampleIntensityFactor; // lightIntensity = lightIntensity in normal direction (often called I0) -> seen area is smaller to the border
				vec3 bsdf = BSDF(_viewDir, -lightRay.Direction, _materialData, _normal, pdf);

				vec3 irradiance = (surfaceCos / lightDistSq) * lightSampleIntensity;
				return irradiance * bsdf;
			}
		}
	}

	return vec3(0.0);
}
