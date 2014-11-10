// Sample hemisphere with cosine density.
//    randomSample is a random number between 0-1
vec3 SampleUnitHemisphere(vec2 randomSample, vec3 U, vec3 V, vec3 W)
{
    float phi = PI_2 * randomSample.x;
    float r = sqrt(randomSample.y);
    float x = r * cos(phi);
    float y = r * sin(phi);
    //float z = sqrt(saturate(1.0 - x*x - y*y));
	float z = 1.0 - x*x -y*y;
    z = z > 0.0 ? sqrt(z) : 0.0;

    return x*U + y*V + z*W;
}


// Sample Phong lobe relative to U, V, W frame
vec3 SamplePhongLobe(vec2 randomSample, float exponent, vec3 U, vec3 V, vec3 W)
{
	float power = exp(log(randomSample.y) / (exponent+1.0f));
	float phi = randomSample.x * 2.0 * PI;
	float scale = sqrt(1.0 - power*power);

	float x = cos(phi)*scale;
	float y = sin(phi)*scale;
	float z = power;

	return x*U + y*V + z*W;
}

// Sample a direction from the custom surface BRDF
// weight x/p for the monte carlo integral. x weights the color channles/is the brdf
vec3 SampleBRDF(vec3 incidentDirection, int material, inout uint seed, vec3 U, vec3 V, vec3 N, vec2 texcoord, out vec3 weight)
{
	// THIS TWO LINES ARE ONLY HERE UNTIL THE OTHER STUFF RUNS
	//weight = vec3(0.5);
	//return SampleUnitHemisphere(Random2(seed), U, V, N);

	vec4 reflectiveness = textureLod(sampler2D(Materials[material].reflectivenessTexHandle), texcoord, 0.0);

	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	float cosTheta = -dot(N, incidentDirection);
	vec3 preflect = Materials[material].Fresnel0 + Materials[material].Fresnel1 * pow(saturate(1.0 - abs(cosTheta)), 5.0);
	preflect *= reflectiveness.xyz;
	float avgPReflect = (preflect.x + preflect.y + preflect.z) / 3.0;
	if( avgPReflect > Random(seed) )
	{
		// Reflect
		vec3 sampleDir = normalize(incidentDirection + 2 * cosTheta * N);
		// Normalize the probability
		float phongNormalization = (reflectiveness.w + 2.0) / 6.283185307;
		weight = phongNormalization * preflect / avgPReflect;
		// Create phong sample in reflection direction
		vec3 RU, RV;
		CreateONB(sampleDir, RU, RV);
		vec3 phongDir = SamplePhongLobe(Random2(seed), reflectiveness.w, RU, RV, sampleDir);
		// Rejection sampling (crashes driver)
		//while(dot(phongDir, N) <= 0)
		//	phongDir = SamplePhongLobe(Random2(seed), reflectiveness.w, RU, RV, sampleDir);
		return phongDir;
		// TODO Resample in negative half space
	} else {
		// Refract/Absorb/Diffus
		vec3 opacity = textureLod(sampler2D(Materials[material].opacityTexHandle), texcoord, 0.0).xyz;
		float avgOpacity = (opacity.x + opacity.y + opacity.z) / 3.0;
		if( avgOpacity > Random(seed) )
		{
			vec3 diffuse = textureLod(sampler2D(Materials[material].diffuseTexHandle), texcoord, 0.0).xyz;
			// Normalize the probability
			vec3 pdiffuse = (1.0 - preflect) * opacity;
			float avgPDiffuse = (pdiffuse.x + pdiffuse.y + pdiffuse.z) / 3.0;
			weight = (diffuse * pdiffuse) / avgPDiffuse;
			// Create diffuse sample
			return SampleUnitHemisphere(Random2(seed), U, V, N);
		} else {
			// Refract
			vec3 sampleDir;
			float eta = cosTheta > 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg;
			float sinTheta2Sq = eta * eta * (1.0f - cosTheta * cosTheta);
			// Total reflection
			if( sinTheta2Sq >= 1.0f )
				sampleDir = normalize(incidentDirection + 2 * cosTheta * N);
			else sampleDir = normalize(eta * incidentDirection - (sign(cosTheta) * (eta * cosTheta + sqrt(saturate(1.0 - sinTheta2Sq)))) * N);
			// Normalize the probability
			vec3 prefract = (1.0 - preflect) * (1.0 - opacity);
			float avgPRefract = (prefract.x + prefract.y + prefract.z + 1e-5) / 3.00001;
			float phongNormalization = (reflectiveness.w + 2.0) / 6.283185307;
			weight = phongNormalization * prefract / avgPRefract;
			// Create phong sample in reflection direction
			vec3 RU, RV;
			CreateONB(sampleDir, RU, RV);
			return SamplePhongLobe(Random2(seed), reflectiveness.w, RU, RV, sampleDir);
		}
	}
}

// Sample the custom surface BRDF for two known direction
vec3 BRDF(vec3 incidentDirection, vec3 excidentDirection, int material, vec3 N, vec2 texcoord)
{
	vec3 excidentLight = vec3(0.0);
	vec4 reflectiveness = textureLod(sampler2D(Materials[material].reflectivenessTexHandle), texcoord, 0.0);

	float cosTheta = -dot(N, incidentDirection);

	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	vec3 preflect = Materials[material].Fresnel0 + Materials[material].Fresnel1 * pow(saturate(1.0 - abs(cosTheta)), 5.0);
	preflect *= reflectiveness.xyz;
	// Reflection vector
	vec3 sampleDir = normalize(incidentDirection + 2 * cosTheta * N);
	float phongNormalization = (reflectiveness.w + 2.0) / 6.283185307;
	excidentLight += phongNormalization * preflect * pow(max(0.0,dot(sampleDir, excidentDirection)), reflectiveness.w);

	// Refract/Absorb/Diffus
	vec3 opacity = textureLod(sampler2D(Materials[material].opacityTexHandle), texcoord, 0.0).xyz;
	vec3 prefract = 1.0 - preflect;
	vec3 pdiffuse = prefract * opacity;
	prefract *= 1.0 - opacity;
	vec3 diffuse = textureLod(sampler2D(Materials[material].diffuseTexHandle), texcoord, 0.0).xyz;
	excidentLight += pdiffuse * diffuse / 3.141592654;// / pi?

	// Refract
	float eta = cosTheta > 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg;
	float sinTheta2Sq = eta * eta * (1.0f - cosTheta * cosTheta);
	// Test if no total reflection happens
	if( sinTheta2Sq < 1.0f )
		sampleDir = normalize(eta * incidentDirection - (sign(cosTheta) * (eta * cosTheta + sqrt(saturate(1.0 - sinTheta2Sq)))) * N);
	// else the sampleDir is again the reflection vector
	excidentLight += phongNormalization * prefract * pow(max(0.0,dot(sampleDir, excidentDirection)), reflectiveness.w);
	
	return excidentLight;
}