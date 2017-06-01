#define DIVISOR_EPSILON 1e-5

//default is no reflection
#define USE_CONSTANT_REFLECTION
//#define USE_FRESNEL
//#define APPLY_FRESNEL_CORRECTION

// Data from material textures.
struct MaterialData
{
	vec4 Reflectiveness;
	vec3 Opacity;
	vec3 Diffuse;
	vec3 Emissivity;	// Self emissivity luminance
	vec3 Fresnel0;	// Fist precomputed coefficient for fresnel approximation (rgb)
	float RefractionIndexAvg;
	vec3 Fresnel1;	// Second precomputed coefficient for fresnel approximation (rgb)
};
MaterialData SampleMaterialData(int materialID, vec2 texcoord)
{
	MaterialData materialData;
	materialData.Reflectiveness = textureLod(sampler2D(Materials[materialID].reflectivenessTexHandle), texcoord, 0.0);
	materialData.Opacity = textureLod(sampler2D(Materials[materialID].opacityTexHandle), texcoord, 0.0).xyz;
	materialData.Diffuse = textureLod(sampler2D(Materials[materialID].diffuseTexHandle), texcoord, 0.0).xyz;
	materialData.Emissivity = textureLod(sampler2D(Materials[materialID].emissivityTexHandle), texcoord, 0.0).xyz;
	materialData.Fresnel0 = Materials[materialID].Fresnel0;
	materialData.RefractionIndexAvg = Materials[materialID].RefractionIndexAvg;
	materialData.Fresnel1 = Materials[materialID].Fresnel1;
	return materialData;
}


float AvgProbability(vec3 colorProbability)
{
	return (colorProbability.x + colorProbability.y + colorProbability.z + DIVISOR_EPSILON) / (3.0 + DIVISOR_EPSILON);
}

float Avg(vec3 colorProbability)
{
	return (colorProbability.x + colorProbability.y + colorProbability.z ) / 3.0;
}

void GetBSDFDecisionPropabilities(MaterialData materialData, float cosThetaAbs,
									out vec3 preflect, out vec3 prefract, out vec3 pdiffuse)
{
#	if defined(USE_FRESNEL)
		// Compute reflection probabilities with rescaled fresnel approximation from:
		// "Fresnel Term Approximations for Metals" 2005
		// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
		// = Fresnel0 + Fresnel1 * (1-cos theta)^5
		preflect = materialData.Reflectiveness.xyz * (materialData.Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + materialData.Fresnel0);
#	elif defined(USE_CONSTANT_REFLECTION)
		// use constant reflection defined by the parameter specified in the material
		// this is uses to disable Fresnel reflection for debugging..
		preflect = materialData.Fresnel0;
#	else
		preflect = vec3(0.0);
#	endif

	prefract = vec3(1.0) - saturate(preflect);
	pdiffuse = prefract * materialData.Opacity;
	prefract = prefract - pdiffuse;
}

vec3 GetBSDFDecisionPropabilityCorrectionSpecular(MaterialData materialData, float cosThetaAbs, float cosThetaOut)
{
#	if defined(USE_FRESNEL) && defined(APPLY_FRESNEL_CORRECTION)
		return (materialData.Fresnel1 * pow(1.0 - max(cosThetaAbs, cosThetaOut), 5.0) + materialData.Fresnel0) /
		(materialData.Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + materialData.Fresnel0);
#	else
		return vec3(1.0);
#	endif
}

vec3 GetBSDFDecisionPropabilityCorrectionDiffuse(MaterialData materialData, float cosThetaAbs, float cosThetaOut)
{
#	if defined(USE_FRESNEL) && defined(APPLY_FRESNEL_CORRECTION)
		return (1.0 - (materialData.Fresnel1 * pow(1.0 - max(cosThetaAbs, cosThetaOut), 5.0) + materialData.Fresnel0)) /
		(1.0 - (materialData.Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + materialData.Fresnel0));
#	else
		return vec3(1.0);
#	endif
}

vec3 GetDiffuseProbability(MaterialData materialData, float cosThetaAbs)
{
	vec3 preflect;
#	if defined(USE_FRESNEL)
		preflect = materialData.Reflectiveness.xyz * (materialData.Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + materialData.Fresnel0);
#	elif defined(USE_CONSTANT_REFLECTION)
		preflect = materialData.Fresnel0;
#	else
		preflect = vec3(0.0);
#	endif
	vec3 pdiffuse = (vec3(1.0) - saturate(preflect)) * materialData.Opacity;
	return pdiffuse;
}

vec3 GetDiffuseReflectance(MaterialData materialData, float cosThetaAbs)
{
	return GetDiffuseProbability(materialData, cosThetaAbs) * materialData.Diffuse;
}

// Note: While BRDFs need to be symmetric to be physical, BSDFs have no such restriction!
// The restriction is actually f(i->o) / (refrIndex_o²) = f(o->i) / (refrIndex_i²)
// (See Veach PhD Thesis formula 5.14)

// Note: Our BSDF formulation avoids any use of dirac distributions.
// Thus there are no special cases for perfect mirrors or perfect refraction.
// Dirac distributions can only be sampled, evaluating their

// Sample a direction from the custom surface BSDF
// pathThroughput *= brdf * cos(Out, N) / pdf
vec3 __SampleBSDF(vec3 incidentDirection, MaterialData materialData, inout uint seed, vec3 N, inout vec3 pathThroughput, const bool adjoint, out float pdf
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

	// Probabilities - colors
	vec3 preflect, prefract, pdiffuse;
	GetBSDFDecisionPropabilities(materialData, cosThetaAbs, preflect, prefract, pdiffuse);
	// Probabilities - average
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
		//vec3 brdf = (materialData.Diffuse * pdiffuse) / PI;
		//pathThroughput *= brdf * dot(N, outDir) / pdf;
		pathThroughput *= (materialData.Diffuse * pdiffuse) / avgPDiffuse; // Divide with decision probability (avgPDiffuse) for russian roulette.

		// Create diffuse sample
		vec3 U, V;
		CreateONB(N, U, V);
		vec3 outDir = SampleHemisphereCosine(randomSamples, U, V, N);

		float cosThetaOut = dot(N, outDir);
		pdf = saturate(cosThetaOut) / PI;

		// if USE_FRESNEL and APPLY_FRESNEL_CORRECTION is enabled, this will correct the probability for selecting diffuse and specular paths
		pathThroughput *= GetBSDFDecisionPropabilityCorrectionDiffuse(materialData, cosThetaAbs, abs(cosThetaOut));
		//pathThroughput *= abs(cosThetaOut); // do not do this!
		return outDir;
	}
	// Refract:
	else if(avgPDiffuse + avgPRefract > pathDecisionVar)
	{
		// Phong (sampled at end of function) is now sampling refraction.
		pphong = prefract;
		avgPPhong = avgPRefract;

		// Compute refraction direction.
		float eta = cosTheta < 0.0 ? 1.0/materialData.RefractionIndexAvg : materialData.RefractionIndexAvg;
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

			refractEvent = true;
		}
	}

	// Reflect/Refract ray generation

	// Create phong sample in reflection/refraction direction

	reflectRefractDir = normalize(reflectRefractDir);
	vec3 RU, RV;
	CreateONB(reflectRefractDir, RU, RV);

	// Rejection sampling to avoid paths which go into the wrong half space of the surface.
	bool plausible = false;
	vec3 outDir;
	int counter = 0;
	while (!plausible)
	{
		counter++;
		outDir = SamplePhongLobe(randomSamples, materialData.Reflectiveness.w, RU, RV, reflectRefractDir);
		plausible = dot(reflectRefractDir, N) * dot(outDir, N) >= 0.0;
		plausible = true; // disables rejection sampling
		if(!plausible)
			randomSamples = Random2(seed);
		if(counter > 100)
		{
			plausible = true;
			pathThroughput = vec3(0.0);
		}
	}
	
	if(!adjoint)
		pathThroughput *= abs(dot(N, outDir)) / abs(dot(N, reflectRefractDir));

	// Normalize the probability
	//float phongNormalization = (materialData.Reflectiveness.w + 2.0) / (materialData.Reflectiveness.w + 1.0);

	float cosThetaOutAbs = abs(dot(outDir, reflectRefractDir));
	pdf = (materialData.Reflectiveness.w + 1.0) / PI_2 * pow(cosThetaOutAbs, materialData.Reflectiveness.w);
	pdf *= float(counter);
	// vec3 bsdf = (materialData.Reflectiveness.w + 1.0) / PI_2 * pow(abs(dot(outDir, reflectRefractDir)), materialData.Reflectiveness.w) * pphong / avgPPhong / dot(N, outDir)

	// The 1/dot(N, outDir) factor is rather magical: If we would only sample a dirac delta,
	// we would expect that the scattering (rendering) equation would yield the radiance from the (via dirac delta) sampled direction.
	// To achieve this the BRDF must be "dirac/cos(thetai)". Here we want to "smooth out" the dirac delta to a phong lobe, but keep the cos(thetai)=dot(N,outDir) ...
	// See also "Physically Based Rendering" page 440.

	// pathThroughput = bsdf * dot(N, outDir) / pdf;
	pathThroughput *= pphong / avgPPhong; // Divide with decision probability (avgPPhong) for Russian roulette.

	// this deals with the same problem as the Adjoint cases in refractive paths: the ray density is changing
	// when we have a non-perfect reflection, i.e., the "width" of a ray bundle changes during reflection
	// so in case of non diffuse reflection we need to compensate the rate of change
	// the refract case is already corrected
	//if (!refractEvent && !adjoint)
	//	pathThroughput *= abs(dot(N, outDir)) / cosThetaAbs;

#ifdef SAMPLING_DECISION_OUTPUT
	isReflectedDiffuse = false;
#endif

	
	// if USE_FRESNEL and APPLY_FRESNEL_CORRECTION is enabled, this will correct the probability for selecting diffuse and specular paths
	pathThroughput *= GetBSDFDecisionPropabilityCorrectionSpecular(materialData, cosThetaAbs, cosThetaOutAbs);
	// pathThroughput *= cosThetaOutAbs; // do not do this!
	return outDir;
}

vec3 SampleBSDF(vec3 incidentDirection, MaterialData materialData, inout uint seed, vec3 N, inout vec3 pathThroughput
#ifdef SAMPLING_DECISION_OUTPUT
		, out bool isReflectedDiffuse
#endif
		, out float pdf) {
#ifdef SAMPLING_DECISION_OUTPUT
	return __SampleBSDF(incidentDirection, materialData, seed, N, pathThroughput, false, pdf, isReflectedDiffuse);
#else
	return __SampleBSDF(incidentDirection, materialData, seed, N, pathThroughput, false, pdf);
#endif
}

vec3 SampleAdjointBSDF(vec3 incidentDirection, MaterialData materialData, inout uint seed, vec3 N, inout vec3 pathThroughput
#ifdef SAMPLING_DECISION_OUTPUT
		, out bool isReflectedDiffuse
#endif
		, out float pdf) {
#ifdef SAMPLING_DECISION_OUTPUT
	return __SampleBSDF(incidentDirection, materialData, seed, N, pathThroughput, true, pdf, isReflectedDiffuse);
#else
	return __SampleBSDF(incidentDirection, materialData, seed, N, pathThroughput, true, pdf);
#endif
}

// Sample the custom surface BSDF for two known direction
// incidentDirection points to the surface
// excidentDirection points away from the surface
vec3 __BSDF(vec3 incidentDirection, vec3 excidentDirection, MaterialData materialData, vec3 N, out float pdf, bool adjoint, bool specularOnly)
{
	vec3 bsdf = vec3(0.0);
	float cosTheta = dot(N, incidentDirection);
	float cosThetaAbs = saturate(abs(cosTheta));
	float cosThetaOut = dot(N, excidentDirection);

	// Probabilities.
	vec3 preflect, prefract, pdiffuse;
	
#	if defined(USE_FRESNEL) && defined(APPLY_FRESNEL_CORRECTION)
		// inspired PRT Book p. 551
		GetBSDFDecisionPropabilities(materialData, max(saturate(abs(dot(N, excidentDirection))), cosThetaAbs), preflect, prefract, pdiffuse);
#	else
		GetBSDFDecisionPropabilities(materialData, cosThetaAbs, preflect, prefract, pdiffuse);
#	endif

	// Constants for phong sampling.
	float phongFactor_brdf = (materialData.Reflectiveness.w + 1.0) / (2 * PI_2 * abs(cosThetaOut)+DIVISOR_EPSILON); // normalization / abs(cosThetatOut)
	//float phongFactor_brdf = (materialData.Reflectiveness.w + 1.0) / (PI_2); // normalization
	float phongNormalization_pdf = (materialData.Reflectiveness.w + 1.0) / PI_2;

	// Reflection
	vec3 reflectRefractDir = normalize(incidentDirection - (2.0 * cosTheta) * N);
	float reflectionDotExcident_powN = pow(saturate(dot(reflectRefractDir, excidentDirection)), materialData.Reflectiveness.w);
	float reflrefrBRDFFactor = phongFactor_brdf * reflectionDotExcident_powN;
	float reflrefrPDFFactor = phongNormalization_pdf * reflectionDotExcident_powN;

	// plausibility
	//if(dot(reflectRefractDir, N) * dot(excidentDirection, N) >= 0.0)
		bsdf = preflect * reflrefrBRDFFactor;

	// this deals with the same problem as the Adjoint cases in refractive paths: the ray density is changing
	// when we have a non-perfect reflection, i.e., the "width" of a ray bundle changes during reflection
	// so in case of non diffuse reflection we need to compensate the rate of change
	// the refract case is already corrected
	//if (!adjoint)
		bsdf *= abs(cosThetaOut) / cosThetaAbs;

	pdf = AvgProbability(preflect) * reflrefrPDFFactor;

	// Diffuse
	if(!specularOnly)
	{
		bsdf += pdiffuse * materialData.Diffuse / PI;
		pdf += Avg(pdiffuse) * saturate(cosThetaOut) / PI;// TODO: Why is material.Diffuse not used here? What if material black?
	}

#ifdef STOP_ON_DIFFUSE_BOUNCE
	// No refractive next event estimation
	return bsdf;
#endif

	//if(cosTheta * cosThetaOut > 0.0) return bsdf;
	// Refract
	float eta = cosTheta < 0.0 ? 1.0/materialData.RefractionIndexAvg : materialData.RefractionIndexAvg;
	float etaSq = eta*eta;
	float sinTheta2Sq = etaSq * (1.0f - cosTheta * cosTheta);

	// No total reflection?
	if( sinTheta2Sq < 1.0f )
	{
		// N * sign(cosTheta) = "faceForwardNormal"; -cosThetaAbs = -dot(faceForwardNormal, incidentDirection)
		reflectRefractDir = normalize(eta * incidentDirection + (sign(cosTheta) * (sqrt(saturate(1.0 - sinTheta2Sq)) - eta * cosThetaAbs)) * N);

		float refractionDotExcident_powN = pow(saturate(dot(reflectRefractDir, excidentDirection)), materialData.Reflectiveness.w);

		reflrefrBRDFFactor = phongFactor_brdf * refractionDotExcident_powN;
		reflrefrPDFFactor = phongNormalization_pdf * refractionDotExcident_powN;

		if(!adjoint)
			reflrefrBRDFFactor /= etaSq;

		reflrefrBRDFFactor *= abs(dot(N, excidentDirection)) / abs(dot(N, reflectRefractDir));
	}
	// else the sampleDir is again the reflection vector

	// plausibility
	//if (dot(reflectRefractDir, N) * dot(excidentDirection, N) >= 0.0)
		bsdf += prefract * reflrefrBRDFFactor;

	pdf += AvgProbability(prefract) * reflrefrPDFFactor;

	return bsdf;
}

vec3 BSDF(vec3 incidentDirection, vec3 excidentDirection, MaterialData materialData, vec3 N, out float pdf)
{
	return __BSDF(incidentDirection, excidentDirection, materialData, N, pdf, false, false);
}

vec3 BSDFSpecular(vec3 incidentDirection, vec3 excidentDirection, MaterialData materialData, vec3 N, out float pdf)
{
	return __BSDF(incidentDirection, excidentDirection, materialData, N, pdf, false, true);
}

// Adjoint in the sense of Veach PhD Thesis 3.7.6
// Use this for tracing light particles, use BSDF for tracing camera paths.
vec3 AdjointBSDF(vec3 incidentDirection, vec3 excidentDirection, MaterialData materialData, vec3 N, out float pdf)
{
	return __BSDF(-excidentDirection, -incidentDirection, materialData, N, pdf, true, false);
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

	return saturate(abs(dot(incomingLight, shadingNormal) * dot(toCamera, geometryNormal)) / max(abs(dot(incomingLight, geometryNormal)) , DIVISOR_EPSILON));

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

	return saturate(abs(dot(inDir, shadingNormal) * dot(outDir, geometryNormal)) / max(abs(dot(inDir, geometryNormal) * dot(outDir, shadingNormal)), DIVISOR_EPSILON));
}
