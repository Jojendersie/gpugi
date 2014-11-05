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

// Sample the custom surface BRDF
vec3 SampleBRDF(vec3 incidentDirection, int material, inout uint seed, vec3 U, vec3 V, vec3 N, vec2 texcoord, out vec3 p)
{
	vec4 reflectiveness = textureLod(sampler2D(Materials[material].reflectivenessTexHandle), texcoord, 0.0);

	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	float cosTheta = dot(N, incidentDirection);
	vec3 preflect = Materials[material].Fresnel0 + Materials[material].Fresnel1 * pow(1.0 - cosTheta, 5.0);
	preflect *= reflectiveness;

	float avgPReflect = (preflect.x + preflect.y + preflect.z) / 3.0;
	if( avgPReflect >= Random(seed) )
	{
		// Reflect
		vec3 sampleDir = normalize(incidentDirection + 2 * cosTheta * N);
		// Normalize the probability
		p = avgPReflect / preflect;
		// Create phong sample in reflection direction
		vec3 RU, RV;
		CreateONB(sampleDir, RU, RV);
		return SamplePhongLobe(Random2(seed), reflectiveness.w, RU, RV, sampleDir);
		// TODO Resample in negative half space
	} else {
		// Refract/Absorb/Diffus
		vec3 opacity = textureLod(sampler2D(Materials[material].opacityTexHandle), texcoord, 0.0).xyz;
		vec3 prefract = 1.0 - preflect;
		vec3 pdiffuse = prefract * opacity;
		prefract *= 1.0 - opacity;
		float avgPDiffuse = (pdiffuse.x + pdiffuse.y + pdiffuse.z) / 3.0;
		if( avgPDiffuse >= Random(seed) )
		{
			vec3 diffuse = textureLod(sampler2D(Materials[material].diffuseTexHandle), texcoord, 0.0).xyz;
			// Normalize the probability
			p = avgPDiffuse / (diffuse * pdiffuse);
			// Create diffuse sample
			return SampleUnitHemisphere(Random2(seed), U, V, N);
		} else {
			// Refract
			vec3 sampleDir;
			float ratio = cosTheta < 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg;
			float sinTheta2Sq = ratio * ratio * (1.0f - cosTheta * cosTheta);
			// Total reflection
			if( sinTheta2Sq >= 1.0f ) sampleDir = normalize(incidentDirection + 2 * cosTheta * N);
			else sampleDir = normalize(ratio * incidentDirection - (sign(cosTheta) * (ratio * abs(cosTheta) - sqrt(1.0 - sinTheta2Sq))) * N);
			// Normalize the probability
			p = ((prefract.x + prefract.y + prefract.z).x / 3.0).xxx / prefract;
			// Create phong sample in reflection direction
			vec3 RU, RV;
			CreateONB(sampleDir, RU, RV);
			return SamplePhongLobe(Random2(seed), reflectiveness.w, RU, RV, sampleDir);
		}
	}

	// Russian roulette between color chanels
	const vec3 W = vec3(0.2125, 0.7154, 0.0721);
	//diffuse *= 
	
	return vec3(0,0,0);
}