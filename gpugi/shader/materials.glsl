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

vec3 GetDiffuseReflectance(int material, MaterialTextureData materialTexData, float cosThetaAbs)
{
	vec3 preflect = materialTexData.Reflectiveness.xyz * (Materials[material].Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + Materials[material].Fresnel0);
	vec3 pdiffuse = (vec3(1.0) - saturate(preflect)) * materialTexData.Opacity;
	return pdiffuse * materialTexData.Diffuse;
}

vec3 GetDiffuseProbability(int material, MaterialTextureData materialTexData, float cosThetaAbs)
{
	vec3 preflect = materialTexData.Reflectiveness.xyz * (Materials[material].Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + Materials[material].Fresnel0);
	vec3 pdiffuse = (vec3(1.0) - saturate(preflect)) * materialTexData.Opacity;
	return pdiffuse;
}

// Note: While BRDFs need to be symmetric to be physical, BSDFs have no such restriction!
// The restriction is actually f(i->o) / (refrIndex_o²) = f(o->i) / (refrIndex_i²)
// (See Veach PhD Thesis formular 5.14)

// Note: Our BSDF formulation avoids any use of dirac distributions.
// Thus there are no special cases for perfect mirrors or perfect refraction.
// Dirac distributions can only be sampled, evaluating their 

// Sample a direction from the custom surface BSDF
// pathThroughput *= brdf * cos(Out, N) / pdf
vec3 __SampleBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput, 		const bool adjoint, out float pdf
	#ifdef SAMPLING_DECISION_OUTPUT
		, out bool isReflectedDiffuse
	#endif
	)
{
	// Probability model:
	//        
	// 0                             1
	// |---------|---------|---------|
	//    Diff   |  Refr   | Reflect
	//
	// pathDecisionVar points now somewhere between 0 and 1.
	// Decisions are taken using russian roulette.

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

	// Diffuse:
	if(avgPDiffuse > pathDecisionVar)	
	{
#ifdef SAMPLING_DECISION_OUTPUT
		isReflectedDiffuse = true;
#endif
#ifdef STOP_ON_DIFFUSE_BOUNCE
		pathThroughput = vec3(-1.0, 0.0, 0.0);
		return vec3(0.0, 0.0, 0.0);
#endif
		//vec3 brdf = (materialTexData.Diffuse * pdiffuse) / PI;
		//pathThroughput *= brdf * dot(N, outDir) / pdf;
		pathThroughput *= (materialTexData.Diffuse * pdiffuse) / avgPDiffuse; // Divide with decision probability (avgPDiffuse) for russian roulette.

		// Create diffuse sample
		vec3 U, V;
		CreateONB(N, U, V);
		vec3 outDir = SampleHemisphereCosine(randomSamples, U, V, N);

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
	//float phongNormalization = (materialTexData.Reflectiveness.w + 2.0) / (materialTexData.Reflectiveness.w + 1.0);

	pdf = (materialTexData.Reflectiveness.w + 1.0) / PI_2 * pow(abs(dot(outDir, reflectRefractDir)), materialTexData.Reflectiveness.w);
	// vec3 bsdf = (materialTexData.Reflectiveness.w + 1.0) / PI_2 * pow(abs(dot(outDir, reflectRefractDir)), materialTexData.Reflectiveness.w) * pphong / avgPPhong / dot(N, outDir)
	
	// The 1/dot(N, outDir) factor is rather magical: If we would only sample a dirac delta,
	// we would expect that the scattering (rendering) equation would yield the radiance from the (via dirac delta) sampled direction.
	// To achieve this the BRDF must be "dirac/cos(thetai)". Here we want to "smooth out" the dirac delta to a phong lobe, but keep the cos(thetai)=dot(N,outDir) ...
	// See also "Physically Based Rendering" page 440.

	// pathThroughput = bsdf * dot(N, outDir) / pdf;
	pathThroughput *= pphong / avgPPhong; // Divide with decision probability (avgPPhong) for russian roulette.

#ifdef SAMPLING_DECISION_OUTPUT
	isReflectedDiffuse = false;
#endif

	return outDir;
}

vec3 SampleBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput
#ifdef SAMPLING_DECISION_OUTPUT
		, out bool isReflectedDiffuse
#endif
#ifdef SAMPLEBSDF_OUTPUT_PDF
		, out float pdf) {
#else 
	) { float pdf; // Hopefully removed by optimizer if not needed
#endif
#ifdef SAMPLING_DECISION_OUTPUT
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, false, pdf, isReflectedDiffuse);
#else
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, false, pdf);
#endif
}

vec3 SampleAdjointBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput
#ifdef SAMPLING_DECISION_OUTPUT
		, out bool isReflectedDiffuse
#endif
#ifdef SAMPLEBSDF_OUTPUT_PDF
		, out float pdf) {
#else 
	) { float pdf; // Hopefully removed by optimizer if not needed
#endif
#ifdef SAMPLING_DECISION_OUTPUT
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, true, pdf, isReflectedDiffuse);
#else
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, true, pdf);
#endif
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
	float phongFactor_brdf = (materialTexData.Reflectiveness.w + 1.0) / (2 * PI_2 * abs(cosThetaOut)+DIVISOR_EPSILON); // normalization / abs(cosThetatOut)
	//float phongFactor_brdf = (materialTexData.Reflectiveness.w + 1.0) / (2 * PI_2); // normalization / abs(cosThetatOut)
	float phongNormalization_pdf = (materialTexData.Reflectiveness.w + 1.0) / PI_2;

	// Reflection
	vec3 reflectRefractDir = normalize(incidentDirection - (2.0 * cosTheta) * N);
	float reflectionDotExcident_powN = pow(saturate(dot(reflectRefractDir, excidentDirection)), materialTexData.Reflectiveness.w);
	float reflrefrBRDFFactor = phongFactor_brdf * reflectionDotExcident_powN;
	float reflrefrPDFFactor = phongNormalization_pdf * reflectionDotExcident_powN;
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


		float refractionDotExcident_powN = pow(saturate(dot(reflectRefractDir, excidentDirection)), materialTexData.Reflectiveness.w);
		
		reflrefrBRDFFactor = phongFactor_brdf * refractionDotExcident_powN;
		reflrefrPDFFactor = phongNormalization_pdf * refractionDotExcident_powN;

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



// The (often underestimated) problem:
// The number of light rays hitting a triangle does NOT depend on the shading normal!
// The fix (see also Veach Chapter 5.3 or http://ompf2.com/viewtopic.php?f=3&t=1944):
float AdjointBSDFShadingNormalCorrectedOutDotN(vec3 incomingLight, vec3 toCamera, vec3 geometryNormal, vec3 shadingNormal)
{
	// alternative without if? todo
	if(dot(incomingLight, geometryNormal) * dot(incomingLight, shadingNormal) < 0 ||
		dot(toCamera, geometryNormal) * dot(toCamera, shadingNormal) < 0)
		return 0.0;

	return saturate(abs(dot(incomingLight, shadingNormal) * dot(toCamera, geometryNormal)) / (abs(dot(incomingLight, geometryNormal)) + DIVISOR_EPSILON));

	// Alternative without correction:
	// return saturate(dot(toCamera, shadingNormal));
}
float AdjointBSDFShadingNormalCorrection(vec3 inDir, vec3 outDir, vec3 geometryNormal, vec3 shadingNormal)
{
	//return 1.0f;
	// alternative without if? todo
	if(dot(inDir, geometryNormal) * dot(inDir, shadingNormal) < 0 ||
		dot(outDir, geometryNormal) * dot(outDir, shadingNormal) < 0)
		return 0.0;

	return saturate(abs(dot(inDir, shadingNormal) * dot(outDir, geometryNormal)) / (abs(dot(inDir, geometryNormal) * dot(outDir, shadingNormal)) + DIVISOR_EPSILON));
}
