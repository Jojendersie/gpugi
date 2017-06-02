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

vec3 GetDiffuseReflectance(MaterialData materialData, float cosThetaAbs)
{
	vec3 preflect = materialData.Reflectiveness.xyz * (materialData.Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + materialData.Fresnel0);
	vec3 pdiffuse = (vec3(1.0) - saturate(preflect)) * materialData.Opacity;
	return pdiffuse * materialData.Diffuse;
}

vec3 GetDiffuseProbability(MaterialData materialData, float cosThetaAbs)
{
	vec3 preflect = materialData.Reflectiveness.xyz * (materialData.Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + materialData.Fresnel0);
	vec3 pdiffuse = (vec3(1.0) - saturate(preflect)) * materialData.Opacity;
	return pdiffuse;
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
	// Decisions are taken using Russian roulette.

	float cosThetaIn = dot(N, incidentDirection);
	float cosThetaInAbs = saturate(abs(cosThetaIn));
	float alteredCosThetaIn;
	float alteredCosThetaInAbs;


	// in case of a refraction, the path changes into a medium with higher or lower optical density
	// this also changes the size of the Phong lobe:
	//	-	when entering a denser volume, the possible refraction direction is not an entire hemisphere since we are refracting towards the "normal" 
	//		this means the lobe gets tighter
	//	-	when leaving the denser volume, the opposite happens, the lobe gets wider since we are refracting away from the "normal"
	//	-	in the case of reflection (or total reflection) the density does not change, the micro facets are the same, so the standard lobe can be used

	// to address for this, we sample the lobe a space we know an thats the side of the incident ray
	// this however, is also valid for reflection paths, so we do in general

	vec3 alteredIncidentDirection;			// sampling another incident ray (just to produce correctly distributed reflection and refraction rays)
	vec2 randomSamples = Random2(seed);		// Random values needed for sampling direction

	vec3 RU, RV;							// lobe space
	CreateONB(incidentDirection, RU, RV);

	// rejection sampling
	bool plausible = false;
	int counter = 0;
	while (!plausible)
	{
		counter++;
		alteredIncidentDirection = SamplePhongLobe(randomSamples, materialData.Reflectiveness.w, RU, RV, incidentDirection);
		alteredCosThetaIn = dot(N, alteredIncidentDirection);
		plausible = dot(alteredIncidentDirection, sign(alteredCosThetaIn)*N) >= 0.0; // not below the surface normal
		plausible = true; // disables rejection sampling
		if (!plausible)
			randomSamples = Random2(seed);
		if (counter > 100)
		{
			plausible = true;
			pathThroughput = vec3(0.0);
		}
	}
	alteredCosThetaInAbs = saturate(abs(alteredCosThetaIn));

	// this deals with the same problem as the Adjoint cases in refractive paths: the ray density is changing
	// when we have a non-perfect reflection, i.e., the "width" of a ray bundle changes during reflection
	// so in case of non diffuse reflection we need to compensate the rate of change
	//float rayBundleDensityCorrection = 1.0;
	//float rayBundleDensityCorrection = alteredCosThetaInAbs / abs(dot(N, incidentDirection));
	float rayBundleDensityCorrection = adjoint ? 1.0 : (alteredCosThetaInAbs / abs(dot(N, incidentDirection)));

	// Probabilities - colors
	vec3 pReflect, pRefract, pDiffuse;
	GetBSDFDecisionPropabilities(materialData, cosThetaInAbs, pReflect, pRefract, pDiffuse);
	// Probabilities - average
	float pReflectAvg = AvgProbability(pReflect);
	float pRefractAvg = AvgProbability(pRefract);
	float pDiffuseAvg = Avg(pDiffuse);

	// Assume reflection for phong lobe. (may change to refraction)
	vec3 pphong = pReflect;
	float avgPPhong = pReflectAvg;

	// Reflect
	vec3 alteredReflectRefractDir = alteredIncidentDirection - (2.0 * alteredCosThetaIn) * N; // later normalized, may not be final

	// Choose a random path type.
	float pathDecisionVar = Random(seed);

	// Diffuse:
	if(pDiffuseAvg > pathDecisionVar)
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
		pathThroughput *= (materialData.Diffuse * pDiffuse) / pDiffuseAvg; // Divide with decision probability (avgPDiffuse) for russian roulette.

		// Create diffuse sample
		vec3 U, V;
		CreateONB(N, U, V);
		vec3 outDir = SampleHemisphereCosine(randomSamples, U, V, N);

		float cosThetaOut = dot(N, outDir);
		pdf = saturate(cosThetaOut) / PI;

		// if USE_FRESNEL and APPLY_FRESNEL_CORRECTION is enabled, this will correct the probability for selecting diffuse and specular paths
		pathThroughput *= GetBSDFDecisionPropabilityCorrectionDiffuse(materialData, cosThetaInAbs, abs(cosThetaOut));
		//pathThroughput *= abs(cosThetaOut); // do not do this!
		return outDir;
	}
	// Refract:
	else if(pDiffuseAvg + pRefractAvg > pathDecisionVar)
	{
		// Phong (sampled at end of function) is now sampling refraction.
		pphong = pRefract;
		avgPPhong = pRefractAvg;

		// Compute refraction direction.
		float eta = alteredCosThetaIn < 0.0 ? (1.0/materialData.RefractionIndexAvg) : materialData.RefractionIndexAvg;
		float etaSq = eta * eta;
		float k = 1.0 - etaSq * (1.0 - alteredCosThetaIn * alteredCosThetaIn);

		// No total reflection
		if(k > 0.0)
		{
			// N * sign(cosTheta) = "faceForwardNormal"; -cosThetaAbs = -dot(faceForwardNormal, incidentDirection)
			//alteredReflectRefractDir = eta * alteredIncidentDirection + (sign(alteredCosThetaIn) * (sqrt(saturate(k)) - eta * alteredCosThetaIn)) * N;
			alteredReflectRefractDir = refract(alteredIncidentDirection, -sign(alteredCosThetaIn)*N, eta);
			if (dot(alteredReflectRefractDir, alteredReflectRefractDir) < 0.01)
			{
				pathThroughput = vec3(sqrt(-1.0));
			}


			// Need to scale radiance since angle got "widened" or "squeezed"
			// See Veach PhD Thesis 5.2 (Non-symmetry due to refraction) or "Physically Based Rendering" chapter 8.2.3 (page 442)
			// This is part of the BSDF!
			// See also Physically Based Rendering page 442, 8.2.3 "Specular Transmission"
			if(!adjoint) rayBundleDensityCorrection /= etaSq;
		}
	}

	// normalize
	alteredReflectRefractDir = normalize(alteredReflectRefractDir);

	// ray bundle density correction for reflection and refraction
	pathThroughput *= rayBundleDensityCorrection;

	// Normalize the probability
	//float phongNormalization = (materialData.Reflectiveness.w + 2.0) / (materialData.Reflectiveness.w + 1.0);

	float cosThetaOutAbs = abs(dot(alteredReflectRefractDir, N));
	pdf = (materialData.Reflectiveness.w + 1.0) / PI_2 * pow(abs(dot(incidentDirection, alteredIncidentDirection)), materialData.Reflectiveness.w);
	pdf *= float(counter);
	// vec3 bsdf = (materialData.Reflectiveness.w + 1.0) / PI_2 * pow(abs(dot(outDir, reflectRefractDir)), materialData.Reflectiveness.w) * pphong / avgPPhong / dot(N, outDir)

	// The 1/dot(N, outDir) factor is rather magical: If we would only sample a dirac delta,
	// we would expect that the scattering (rendering) equation would yield the radiance from the (via dirac delta) sampled direction.
	// To achieve this the BRDF must be "dirac/cos(thetai)". Here we want to "smooth out" the dirac delta to a phong lobe, but keep the cos(thetai)=dot(N,outDir) ...
	// See also "Physically Based Rendering" page 440.

	// pathThroughput = bsdf * dot(N, outDir) / pdf;
	pathThroughput *= pphong / avgPPhong; // Divide with decision probability (avgPPhong) for Russian roulette.

#ifdef SAMPLING_DECISION_OUTPUT
	isReflectedDiffuse = false;
#endif

	// if USE_FRESNEL and APPLY_FRESNEL_CORRECTION is enabled, this will correct the probability for selecting diffuse and specular paths
	pathThroughput *= GetBSDFDecisionPropabilityCorrectionSpecular(materialData, alteredCosThetaInAbs, cosThetaOutAbs);
	// pathThroughput *= cosThetaOutAbs; // do not do this!
	return alteredReflectRefractDir;
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
	float cosThetaIn = dot(N, incidentDirection);
	float cosThetaInAbs = saturate(abs(cosThetaIn));
	float cosThetaOut = dot(N, excidentDirection);
	float cosThetaOutAbs = saturate(abs(cosThetaOut));

	// Probabilities
	vec3 pReflect, pRefract, pDiffuse;
	
#	if defined(USE_FRESNEL) && defined(APPLY_FRESNEL_CORRECTION)
		// inspired PRT Book p. 551
		GetBSDFDecisionPropabilities(materialData, max(saturate(abs(dot(N, excidentDirection))), cosThetaAbs), preflect, prefract, pdiffuse);
#	else
		GetBSDFDecisionPropabilities(materialData, cosThetaInAbs, pReflect, pRefract, pDiffuse);
#	endif

	// Constants for phong sampling.
	float phongFactor_brdf = (materialData.Reflectiveness.w + 1.0) / (1.0 * PI_2 * cosThetaOutAbs + DIVISOR_EPSILON); // normalization / abs(cosThetatOut)
	//float phongFactor_brdf = (materialData.Reflectiveness.w + 1.0) / (PI_2); // normalization
	float phongNormalization_pdf = (materialData.Reflectiveness.w + 1.0) / PI_2;

	// Reflection
	//vec3 reflectRefractDir = normalize(incidentDirection - (2.0 * cosThetaIn) * N);
	vec3 reflectRefractDir = normalize(reflect(incidentDirection, N));
	float reflectionDotExcident_powN = pow(saturate(dot(reflectRefractDir, excidentDirection)), materialData.Reflectiveness.w);

	// this deals with the same problem as the Adjoint cases in refractive paths: the ray density is changing
	// when we have a non-perfect reflection, i.e., the "width" of a ray bundle changes during reflection
	// so in case of non diffuse reflection we need to compensate the rate of change
	//float rayBundleDensityCorrection = 1.0;
	float rayBundleDensityCorrection = cosThetaOutAbs / abs(dot(N, reflectRefractDir));
	//float rayBundleDensityCorrection = adjoint ? 1.0 : (cosThetaOutAbs / abs(dot(N, reflectRefractDir)));

	float reflrefrBRDFFactor = phongFactor_brdf * reflectionDotExcident_powN;
	float reflrefrPDFFactor = phongNormalization_pdf * reflectionDotExcident_powN;

	// reflective contribution
	bsdf = pReflect * reflrefrBRDFFactor * rayBundleDensityCorrection;
	pdf = AvgProbability(pReflect) * reflrefrPDFFactor * rayBundleDensityCorrection;

	// Diffuse
	if(!specularOnly)
	{
		// diffuse contribution
		bsdf += pDiffuse * materialData.Diffuse / PI;
		pdf += Avg(pDiffuse) * saturate(cosThetaOut) / PI;// TODO: Why is material.Diffuse not used here? What if material black?
	}

#ifdef STOP_ON_DIFFUSE_BOUNCE
	// No refractive next event estimation
	return bsdf;
#endif

	// Refract
	float eta = (cosThetaIn < 0.0) ? (1.0/materialData.RefractionIndexAvg) : materialData.RefractionIndexAvg;
	float etaSq = eta*eta;
	float k = 1.0 - etaSq * (1.0f - cosThetaIn * cosThetaIn);

	// No (total) reflection?
	if((k >= 0.0) && (dot(sign(cosThetaIn)*N, excidentDirection) > 0.0))
	//if(k > 0.0)
	{
		bsdf = vec3(sqrt(-1.0));
		return bsdf;

		// at this point we have a refraction, the path changes into a medium with higher or lower optical density
		// this also changes the size of the Phong lobe:
		//	-	when entering a denser volume, the possible refraction direction is not an entire hemisphere since we are refracting towards the "normal" 
		//		this means the lobe gets tighter
		//	-	when leaving the denser volume, the opposite happens, the lobe gets wider since we are refracting away from the "normal"
		//	-	in the case of reflection (or total reflection) the density does not change, the micro facets are the same, so the standard lobe can be used

		// the idea to handle this without knowing about the actual size of the lobe after refraction is to compute the lobe in the "air" with known lobe

		// we refract the excident direction back onto the side with no change in density
		vec3 backRefractedExcident = refract(-excidentDirection, sign(cosThetaIn)*N, 1.0/eta);
		if(dot(backRefractedExcident, backRefractedExcident) < 0.01)
		{
			//bsdf = vec3(sqrt(-1.0));
			//return bsdf;
		}
		else
		{
			backRefractedExcident = normalize(backRefractedExcident);

			// get cos(phi)^n weight
			float refractionDotExcident_powN = pow(saturate(dot(backRefractedExcident, -incidentDirection)), materialData.Reflectiveness.w);
			//refractionDotExcident_powN = 0.01f;
			// change the factors
			// in case of total reflection, the values stay the same which leads to combined probability 
			// for reflection and refraction to use the reflection case
			reflrefrBRDFFactor = phongFactor_brdf * refractionDotExcident_powN;
			reflrefrPDFFactor = phongNormalization_pdf * refractionDotExcident_powN;

			//vec3 R = normalize(reflect(incidentDirection, -sign(cosThetaIn) * N));
			//rayBundleDensityCorrection = abs(dot(N, backRefractedExcident)) / abs(dot(N, R));

			// width of the ray bundle also changes because of refraction (influences only view ray, not photons)
			if(!adjoint) rayBundleDensityCorrection /= etaSq;

			// refractive contribution
			bsdf += pRefract * reflrefrBRDFFactor * rayBundleDensityCorrection;
			pdf += AvgProbability(pRefract) * reflrefrPDFFactor * rayBundleDensityCorrection;
		}
	}
	// else the sampleDir is again the reflection vector
	if(k < 0.0)
	{
		// total reflection
		bsdf += pRefract * reflrefrBRDFFactor * rayBundleDensityCorrection;
		pdf += AvgProbability(pRefract) * reflrefrPDFFactor * rayBundleDensityCorrection;
	}

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
