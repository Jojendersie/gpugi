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
vec3 SampleBRDF(vec3 incidentDirection, int material, vec2 randomSample, vec3 U, vec3 V, vec3 N, out vec3 p)
{
	vec3 diffuse; // TODO Load
	vec3 refrN; // TODO Load
	vec3 refrR; // TODO Load
	vec3 opacity; // TODO Load
	vec3 reflectiveness; // TODO Load

	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	vec3 preflect = Materials[material].Fresnel0 + Materials[material].Fresnel1 * pow(1.0 - dot(N, incidentDirection), 5.0);

	// Russian roulette between color chanels
	const vec3 W = vec3(0.2125, 0.7154, 0.0721);
	//diffuse *= 
	
	return vec3(0,0,0);
}