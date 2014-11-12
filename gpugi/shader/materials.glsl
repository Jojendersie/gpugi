#define DIVISOR_EPSILON 1e-5

// Sample hemisphere with cosine density.
//    randomSample is a random number between 0-1
vec3 SampleUnitHemisphere(vec2 randomSample, vec3 U, vec3 V, vec3 W)
{
    float phi = PI_2 * randomSample.x;
    float r = sqrt(randomSample.y);
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(saturate(1.0 - x*x - y*y));
	//float z = 1.0 - x*x -y*y;
    //z = z > 0.0 ? sqrt(z) : 0.0;

    return x*U + y*V + z*W;
}


// Sample Phong lobe relative to U, V, W frame
vec3 SamplePhongLobe(vec2 randomSample, float exponent, vec3 U, vec3 V, vec3 W)
{
	float phi = PI_2 * randomSample.x;
	float power = exp(log(randomSample.y) / (exponent+1.0f));
	float scale = sqrt(1.0 - power*power);

	float x = cos(phi)*scale;
	float y = sin(phi)*scale;
	float z = power;

	return x*U + y*V + z*W;
}

// Sample a direction from the custom surface BRDF
// weight x/p for the monte carlo integral. x weights the color channles/is the brdf
vec3 SampleBRDF(vec3 incidentDirection, int material, vec4 reflectiveness, vec3 opacity, vec3 diffuse, inout uint seed, vec3 N, out vec3 weight)
{
	// THIS TWO LINES ARE ONLY HERE UNTIL THE OTHER STUFF RUNS
	//weight = vec3(0.5);
	//return SampleUnitHemisphere(Random2(seed), U, V, N);


	// Probability model:
	//      avgPReflect  
	// 0         |    avgPDiffuse    1
	// |---------|---------|---------|
	//   Reflect |   Diff     Refr
	//
	// pathDecisionVar points now somewhere between 0 and 1.


	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)�+k�) / ((n+1)�+k�) + 4n / ((n+1)�+k�) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	float cosTheta = -dot(N, incidentDirection);

	// Reflect
	vec3 sampleDir = (2.0 * cosTheta) * N + incidentDirection; // later normalized, may not be final

	// Random values later needed for sampling direction
	vec2 randomSamples = Random2(seed);

	// Reflection probability
	vec3 preflectrefract = reflectiveness.xyz * (Materials[material].Fresnel1 * pow(saturate(1.0 - abs(cosTheta)), 5.0) + Materials[material].Fresnel0);
	float avgPReflectRefract = (preflectrefract.x + preflectrefract.y + preflectrefract.z) / 3.0; // reflection (!) probability

	// Propability for diffuse reflection (= probability of )
	vec3 pdiffuse = -preflectrefract * opacity + opacity; // preflectrefract is reflection propability
	float avgPDiffuse = (pdiffuse.x + pdiffuse.y + pdiffuse.z) / (3.0);

	// Choose a random path type.
	float pathDecisionVar = Random(seed);

	// Refract:
	if(avgPDiffuse < pathDecisionVar)
	{
		// Compute refraction direction.
		float eta = cosTheta > 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg; // Is this a branch? And if yes, how to avoid it?
		float sinTheta2Sq = eta * eta * (1.0f - cosTheta * cosTheta);
		// Total reflection
		if( sinTheta2Sq < 1.0f )
			sampleDir = eta * incidentDirection - (sign(cosTheta) * (eta * cosTheta + sqrt(saturate(1.0 - sinTheta2Sq)))) * N;

		// Refraction probability
		preflectrefract = (1.0 - preflectrefract) * (1.0 - opacity);
		avgPReflectRefract = (preflectrefract.x + preflectrefract.y + preflectrefract.z + DIVISOR_EPSILON) / (3.0 + DIVISOR_EPSILON); // refraction!
	}
	// Diffuse:
	else if(avgPReflectRefract < pathDecisionVar)	
	{
		// Normalize the probability
		weight = (diffuse * pdiffuse) / avgPDiffuse;
		// Create diffuse sample
		vec3 U, V;
		CreateONB(N, U, V);
		return SampleUnitHemisphere(randomSamples, U, V, N);
	}

	// Reflect: (shares this code with refract)

	// Normalize the probability
	float phongNormalization = (reflectiveness.w + 2.0) / (reflectiveness.w + 1.0);
	weight = preflectrefract * (phongNormalization / avgPReflectRefract);// * abs(cosTheta);

	// Create phong sample in reflection/refraction direction
	sampleDir = normalize(sampleDir);
	vec3 RU, RV;
	CreateONB(sampleDir, RU, RV);
	return SamplePhongLobe(randomSamples, reflectiveness.w, RU, RV, sampleDir);

	// Rejection sampling (crashes driver)
	//while(dot(phongDir, N) <= 0)
	//	phongDir = SamplePhongLobe(Random2(seed), reflectiveness.w, RU, RV, sampleDir);
	//return phongDir;
}

// Sample the custom surface BRDF for two known direction
vec3 BRDF(vec3 incidentDirection, vec3 excidentDirection, int material, vec4 reflectiveness, vec3 opacity, vec3 diffuse, vec3 N)
{
	vec3 excidentLight = vec3(0.0);
	float cosTheta = -dot(N, incidentDirection);

	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)�+k�) / ((n+1)�+k�) + 4n / ((n+1)�+k�) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	vec3 preflect = Materials[material].Fresnel0 + Materials[material].Fresnel1 * pow(saturate(1.0 - abs(cosTheta)), 5.0);
	preflect *= reflectiveness.xyz;
	// Reflection vector
	vec3 sampleDir = normalize(incidentDirection + (2.0 * cosTheta) * N);
	float phongNormalization = (reflectiveness.w + 2.0) / 6.283185307;
	excidentLight += preflect * (phongNormalization * pow(max(0.0,dot(sampleDir, excidentDirection)), reflectiveness.w));

	// Refract/Absorb/Diffus
	vec3 prefract = 1.0 - preflect;
	vec3 pdiffuse = prefract * opacity;
	prefract = prefract - opacity * prefract;
	excidentLight += pdiffuse * diffuse / 3.141592654;// / pi?

	// Refract
	float eta = cosTheta > 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg;
	float sinTheta2Sq = eta * eta * (1.0f - cosTheta * cosTheta);
	// Test if no total reflection happens
	if( sinTheta2Sq < 1.0f )
		sampleDir = normalize(eta * incidentDirection - (sign(cosTheta) * (eta * cosTheta + sqrt(saturate(1.0 - sinTheta2Sq)))) * N);
	// else the sampleDir is again the reflection vector
	excidentLight += prefract * (phongNormalization * pow(max(0.0,dot(sampleDir, excidentDirection)), reflectiveness.w));
	
	return excidentLight;
}