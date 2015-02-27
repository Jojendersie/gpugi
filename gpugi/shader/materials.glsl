#define DIVISOR_EPSILON 1e-5

// Data from material textures.
struct MaterialTextureData
{
	vec4 Reflectiveness;
	vec3 Opacity;
	vec3 Diffuse;
};
MaterialTextureData SampleMaterialData(int materialID, vec2 texcoord)
{
	MaterialTextureData materialTexData;
	materialTexData.Reflectiveness = textureLod(sampler2D(Materials[materialID].reflectivenessTexHandle), texcoord, 0.0);
	materialTexData.Opacity = textureLod(sampler2D(Materials[materialID].opacityTexHandle), texcoord, 0.0).xyz;
	materialTexData.Diffuse = textureLod(sampler2D(Materials[materialID].diffuseTexHandle), texcoord, 0.0).xyz;
	return materialTexData;
}


// Sample hemisphere with cosine density.
//    randomSample is a random number between 0-1
vec3 SampleUnitHemisphere(vec2 randomSample, vec3 U, vec3 V, vec3 W)
{
	float phi = PI_2 * randomSample.x;
	float sinTheta = sqrt(randomSample.y);	// sin(acos(sqrt(1-x))) = sqrt(x)
	float x = sinTheta * cos(phi);
	float y = sinTheta * sin(phi);
	float z = sqrt(1.0 - randomSample.y);	// sqrt(1-sin(theta)^2)

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

float AvgProbability(vec3 colorProbability)
{
	return (colorProbability.x + colorProbability.y + colorProbability.z + DIVISOR_EPSILON) / (3.0 + DIVISOR_EPSILON);
}

float Avg(vec3 colorProbability)
{
	return (colorProbability.x + colorProbability.y + colorProbability.z ) / 3.0;
}

void GetBSDFDecisionPropabilities(int material, MaterialTextureData materialTexData, float cosThetaAbs,
									out vec3 preflect, out vec3 prefract, out vec3 pdiffuse)
{
	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	preflect = materialTexData.Reflectiveness.xyz * (Materials[material].Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + Materials[material].Fresnel0);
	prefract = vec3(1.0) - saturate(preflect);
	pdiffuse = prefract * materialTexData.Opacity;
	prefract = prefract - pdiffuse;
}

// Note: While BRDFs need to be symmetric to be physical, BSDFs have no such restriction!
// The restriction is actually f(i->o) / (refrIndex_o²) = f(o->i) / (refrIndex_i²)
// (See Veach PhD Thesis formular 5.14)

// Note: Our BSDF formulation avoids any use of dirac distributions.
// Thus there are no special cases for perfect mirrors or perfect refraction.
// Dirac distributions can only be sampled, evaluating their 

// Sample a direction from the custom surface BSDF
// pathThroughput *= brdf * cos(Out, N) / pdf
vec3 __SampleBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput, const bool adjoint, out float pdf)
{
	// Probability model:
	//        
	// 0                             1
	// |---------|---------|---------|
	//    Diff   |  Refr   | Reflect
	//
	// pathDecisionVar points now somewhere between 0 and 1.
	// Decisions are taken using russion roulette. This mea

	float cosTheta = dot(N, incidentDirection);
	float cosThetaAbs = saturate(abs(cosTheta));

	// Reflect
	vec3 reflectRefractDir = incidentDirection - (2.0 * cosTheta) * N; // later normalized, may not be final

	// Random values later needed for sampling direction
	vec2 randomSamples = Random2(seed);

	// Propabilities - colors
	vec3 preflect, prefract, pdiffuse;
	GetBSDFDecisionPropabilities(material, materialTexData, cosThetaAbs, preflect, prefract, pdiffuse);
	// Propabilites - average
	float avgPReflect = AvgProbability(preflect);
	float avgPRefract = AvgProbability(prefract);
	float avgPDiffuse = Avg(pdiffuse);

	// Assume reflection for phong lobe. (may change to refraction)
	vec3 pphong = preflect;
	float avgPPhong = avgPReflect;
	bool refractEvent = false;

	// Choose a random path type.
	float pathDecisionVar = Random(seed);

#ifdef STOP_ON_DIFFUSE_BOUNCE
	if(avgPDiffuse > 0)
	{
		pathThroughput = vec3(0.0, 0.0, 0.0);
		return vec3(0.0, 0.0, 0.0);
	}
#endif

	// Diffuse:
	if(avgPDiffuse > pathDecisionVar)	
	{
		//vec3 brdf = (materialTexData.Diffuse * pdiffuse) / PI;
		//pathThroughput *= brdf * dot(N, outDir) / pdf;
		pathThroughput *= (materialTexData.Diffuse * pdiffuse) / avgPDiffuse; // Divide with decision propability (avgPDiffuse) for russion roulette.

		// Create diffuse sample
		vec3 U, V;
		CreateONB(N, U, V);
		vec3 outDir = SampleUnitHemisphere(randomSamples, U, V, N);

		pdf = saturate(dot(N, outDir)) / PI;

		return outDir;
	}
	// Refract:
	else if(avgPDiffuse + avgPRefract > pathDecisionVar)
	{
		refractEvent = true;

		// Phong (sampled at end of function) is now sampling refraction.
		pphong = prefract;
		avgPPhong = avgPRefract;

		// Compute refraction direction.		
		float eta = cosTheta < 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg;
		float etaSq = eta * eta;
		float sinTheta2Sq = etaSq * (1.0f - cosTheta * cosTheta);
		
		// No total reflection
		if( sinTheta2Sq < 1.0f )
		{
			// N * sign(cosTheta) = "faceForwardNormal"; -cosThetaAbs = -dot(faceForwardNormal, incidentDirection)
			reflectRefractDir = eta * incidentDirection + (sign(cosTheta) * (sqrt(saturate(1.0 - sinTheta2Sq)) - eta * cosThetaAbs)) * N;
			
			// Need to scale radiance since angle got "widened" or "squeezed"
			// See Veach PhD Thesis 5.2 (Non-symmetry due to refraction) or "Physically Based Rendering" chapter 8.2.3 (page 442)
			// This is part of the BSDF!
			// See also Physically Based Rendering page 442, 8.2.3 "Specular Transmission"
			if(!adjoint)
				pathThroughput /= etaSq;
		}
	}

	// Reflect/Refract ray generation

	// Create phong sample in reflection/refraction direction
	reflectRefractDir = normalize(reflectRefractDir);
	vec3 RU, RV;
	CreateONB(reflectRefractDir, RU, RV);
	vec3 outDir = SamplePhongLobe(randomSamples, materialTexData.Reflectiveness.w, RU, RV, reflectRefractDir);

	// Normalize the probability
	float phongNormalization = (materialTexData.Reflectiveness.w + 2.0) / (materialTexData.Reflectiveness.w + 1.0);

	pdf = (materialTexData.Reflectiveness.w + 1.0) / PI_2 * pow(abs(dot(outDir, reflectRefractDir)), materialTexData.Reflectiveness.w);
	// vec3 bsdf = (materialTexData.Reflectiveness.w + 2.0) / PI_2 * pow(abs(dot(outDir, reflectRefractDir)), materialTexData.Reflectiveness.w) * pphong / avgPPhong / dot(N, outDir)
	
	// The 1/dot(N, outDir) factor is rather magical: If we would only sample a dirac delta,
	// we would expect that the scattering (rendering) equation would yield the radiance from the (via dirac delta) sampled direction.
	// To achieve this the BRDF must be "dirac/cos(thetai)". Here we want to "smooth out" the dirac delta to a phong lobe, but keep the cos(thetai)=dot(N,outDir) ...
	// See also "Physically Based Rendering" page 440.

	// pathThroughput = bsdf * dot(N, outDir) / pdf;
	pathThroughput *= pphong * (phongNormalization / avgPPhong); // Divide with decision propability (avgPPhong) for russian roulette.

	return outDir;
}

vec3 SampleBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput
#ifdef SAMPLEBSDF_OUTPUT_PDF
		, out float pdf) {
#else 
	) { float pdf; // Hopefully removed by optimizer if not needed
#endif
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, false, pdf);
}

vec3 SampleAdjointBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput
#ifdef SAMPLEBSDF_OUTPUT_PDF
		, out float pdf) {
#else 
	) { float pdf; // Hopefully removed by optimizer if not needed
#endif
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, true, pdf);
}

// Sample the custom surface BSDF for two known direction
// incidentDirection points to the surface
// excidentDirection points away from the surface
vec3 __BSDF(vec3 incidentDirection, vec3 excidentDirection, int material, MaterialTextureData materialTexData, vec3 N, out float pdf, bool adjoint)
{
	vec3 bsdf;
	float cosTheta = dot(N, incidentDirection);
	float cosThetaAbs = saturate(abs(cosTheta));
	float cosThetaOut = dot(N, excidentDirection);

	// Propabilities.
	vec3 preflect, prefract, pdiffuse;
	GetBSDFDecisionPropabilities(material, materialTexData, cosThetaAbs, preflect, prefract, pdiffuse);

	// Constants for phong sampling.
	float phongFactor_brdf = (materialTexData.Reflectiveness.w + 2.0) / (PI_2 * (abs(cosThetaOut)+DIVISOR_EPSILON)); // normalization / abs(cosThetatOut)
	float phongNormalization_pdf = (materialTexData.Reflectiveness.w + 1.0) / PI_2;

	// Reflection
	vec3 reflectRefractDir = normalize(incidentDirection - (2.0 * cosTheta) * N);
	float reflectionDotRefraction_powN = pow(saturate(dot(reflectRefractDir, excidentDirection)), materialTexData.Reflectiveness.w);
	float reflrefrBRDFFactor = phongFactor_brdf * reflectionDotRefraction_powN;
	float reflrefrPDFFactor = phongNormalization_pdf * reflectionDotRefraction_powN;
	bsdf = preflect * reflrefrBRDFFactor;
	pdf = AvgProbability(preflect) * reflrefrPDFFactor;

	// Diffuse
	bsdf += pdiffuse * materialTexData.Diffuse / PI;
	pdf += Avg(pdiffuse) * saturate(cosThetaOut) / PI;

#ifdef STOP_ON_DIFFUSE_BOUNCE
	// No refractive next event estimation
	return bsdf;
#endif

	// Refract
	float eta = cosTheta < 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg;
	float etaSq = eta*eta;
	float sinTheta2Sq = etaSq * (1.0f - cosTheta * cosTheta);

	// No total reflection?
	if( sinTheta2Sq < 1.0f )
	{
		// N * sign(cosTheta) = "faceForwardNormal"; -cosThetaAbs = -dot(faceForwardNormal, incidentDirection)
		reflectRefractDir = normalize(eta * incidentDirection + (sign(cosTheta) * (sqrt(saturate(1.0 - sinTheta2Sq)) - eta * cosThetaAbs)) * N);


		float refractionDotRefraction_powN = pow(saturate(dot(reflectRefractDir, excidentDirection)), materialTexData.Reflectiveness.w);
		
		reflrefrBRDFFactor = phongFactor_brdf * refractionDotRefraction_powN;
		reflrefrPDFFactor = phongNormalization_pdf * refractionDotRefraction_powN;

		if(!adjoint)
			reflrefrBRDFFactor /= etaSq;
	}	
	
	// else the sampleDir is again the reflection vector
	bsdf += prefract * reflrefrBRDFFactor;
	pdf += AvgProbability(prefract) * reflrefrPDFFactor;

	return bsdf;
}

vec3 BSDF(vec3 incidentDirection, vec3 excidentDirection, int material, MaterialTextureData materialTexData, vec3 N
#ifdef SAMPLEBSDF_OUTPUT_PDF
		, out float pdf) {
#else 
	) { float pdf; // Hopefully removed by optimizer if not needed
#endif
	return __BSDF(incidentDirection, excidentDirection, material, materialTexData, N, pdf, false);
}

// Adjoint in the sense of Veach PhD Thesis 3.7.6
// Use this for tracing light particles, use BSDF for tracing camera paths.
vec3 AdjointBSDF(vec3 incidentDirection, vec3 excidentDirection, int material, MaterialTextureData materialTexData, vec3 N
#ifdef SAMPLEBSDF_OUTPUT_PDF
		, out float pdf) {
#else 
	) { float pdf; // Hopefully removed by optimizer if not needed
#endif
	return __BSDF(-excidentDirection, -incidentDirection, material, materialTexData, N, pdf, true);
}