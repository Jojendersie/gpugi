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


// Note: While BRDFs need to be symmetric to be physical, BSDFs have no such restriction!
// The restriction is actually f(i->o) / (refrIndex_o²) = f(o->i) / (refrIndex_i²)
// (See Veach PhD Thesis formular 5.14)

// Note: Our BSDF formulation avoids any use of dirac distributions.
// Thus there are no special cases for perfect mirrors or perfect refraction.
// Dirac distributions can only be sampled, evaluating their 


// Sample a direction from the custom surface BSDF
// pathThroughput *= brdf * cos(Out, N) / pdf
vec3 __SampleBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput, const bool adjoint)
{
	// Probability model:
	//      avgPReflect  
	// 0         |    avgPDiffuse    1
	// |---------|---------|---------|
	//   Reflect |   Diff     Refr
	//
	// pathDecisionVar points now somewhere between 0 and 1.

	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	float cosTheta = dot(N, incidentDirection);
	float cosThetaAbs = saturate(abs(cosTheta));

	// Reflect
	vec3 sampleDir = incidentDirection - (2.0 * cosTheta) * N; // later normalized, may not be final

	// Random values later needed for sampling direction
	vec2 randomSamples = Random2(seed);

	// Reflection probability
	vec3 preflectrefract = materialTexData.Reflectiveness.xyz * (Materials[material].Fresnel1 * pow(1.0 - cosThetaAbs, 5.0) + Materials[material].Fresnel0);
	float avgPReflectRefract = AvgProbability(preflectrefract); // reflection (!) probability

	// Propability for diffuse reflection (= probability of )
	vec3 pdiffuse = -preflectrefract * materialTexData.Opacity + materialTexData.Opacity; // preflectrefract is reflection propability
	float avgPDiffuse = Avg(pdiffuse);

	// Choose a random path type.
	float pathDecisionVar = Random(seed);

	// Refract:
	if(avgPDiffuse <= pathDecisionVar)
	{
		// Refraction probability
		preflectrefract = (1.0 - preflectrefract) * (1.0 - materialTexData.Opacity);
		avgPReflectRefract = AvgProbability(preflectrefract); // refraction!

		// Compute refraction direction.		
		float eta = cosTheta < 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg; // Is this a branch? And if yes, how to avoid it?
		float etaSq = eta * eta;
		float sinTheta2Sq = etaSq * (1.0f - cosTheta * cosTheta);
		
		// No total reflection
		if( sinTheta2Sq < 1.0f )
		{
			// N * sign(cosTheta) = "faceForwardNormal"; -cosThetaAbs = -dot(faceForwardNormal, incidentDirection)
			sampleDir = eta * incidentDirection + (sign(cosTheta) * (sqrt(saturate(1.0 - sinTheta2Sq)) - eta * cosThetaAbs)) * N; 
			
			// Need to scale radiance since angle got "widened" or "squeezed"
			// See Veach PhD Thesis 5.2 (Non-symmetry due to refraction) or "Physically Based Rendering" chapter 8.2.3 (page 442)
			// This is part of the BSDF!
			if(!adjoint)
				pathThroughput *= etaSq;
		}
	}
	// Diffuse:
	else if(avgPReflectRefract < pathDecisionVar)	
	{
		//float pdf = dot(N, outDir) / PI; // avgPDiffuse already incorporated
		//vec3 brdf = (materialTexData.Diffuse * pdiffuse) / (avgPDiffuse * PI);
		//pathThroughput *= brdf * dot(N, outDir) / pdf;
		pathThroughput *= (materialTexData.Diffuse * pdiffuse) / avgPDiffuse;

		// Create diffuse sample
		vec3 U, V;
		CreateONB(N, U, V);
		return SampleUnitHemisphere(randomSamples, U, V, N);
	}

	// Reflect/Refract ray generation

	// Create phong sample in reflection/refraction direction
	sampleDir = normalize(sampleDir);
	vec3 RU, RV;
	CreateONB(sampleDir, RU, RV);
	vec3 outDir = SamplePhongLobe(randomSamples, materialTexData.Reflectiveness.w, RU, RV, sampleDir);

	// Normalize the probability
	float phongNormalization = (materialTexData.Reflectiveness.w + 2.0) / (materialTexData.Reflectiveness.w + 1.0);
	// pdf depends on outDir!
	// float pdf = (materialTexData.Reflectiveness.w + 1.0) / PI_2 * pow(dot(N, sampleDir), materialTexData.Reflectiveness.w)
	// vec3 bsdf = (materialTexData.Reflectiveness.w + 2.0) / PI_2 * pow(dot(N, sampleDir), materialTexData.Reflectiveness.w) * preflectrefract / avgPReflectRefract / dot(N, outDir)
	
	// The 1/dot(N, outDir) factor is rather magical: If we would only sample a dirac delta,
	// we would expect that the scattering (rendering) equation would yield the radiance from the (via dirac delta) sampled direction.
	// To achieve this the BRDF must be "dirac/cos(thetai)". Here we want to "smooth out" the dirac delta to a phong lobe, but keep the cos(thetai)=dot(N,outDir) ...
	// See also "Physically Based Rendering" page 440.

	// pathThroughput = bsdf * dot(N, outDir) / pdf;
	pathThroughput *= preflectrefract * (phongNormalization / avgPReflectRefract);

	return outDir;
}


vec3 SampleBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput)
{
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, false);
}

vec3 SampleAdjointBSDF(vec3 incidentDirection, int material, MaterialTextureData materialTexData, inout uint seed, vec3 N, inout vec3 pathThroughput)
{
	return __SampleBSDF(incidentDirection, material, materialTexData, seed, N, pathThroughput, true);
}

// Sample the custom surface BSDF for two known direction
// incidentDirection points to the surface
// excidentDirection points away from the surface
vec3 __BSDF(vec3 incidentDirection, vec3 excidentDirection, int material, MaterialTextureData materialTexData, vec3 N, bool adjoint)
{
	vec3 bsdf = vec3(0.0);
	float cosTheta = dot(N, incidentDirection);
	float cosThetaAbs = saturate(abs(cosTheta));
	float cosThetaOutInv = 1.0 / abs(dot(N, excidentDirection));

	// Compute reflection probabilities with rescaled fresnel approximation from:
	// Fresnel Term Approximations for Metals
	// ((n-1)²+k²) / ((n+1)²+k²) + 4n / ((n+1)²+k²) * (1-cos theta)^5
	// = Fresnel0 + Fresnel1 * (1-cos theta)^5
	vec3 preflect = Materials[material].Fresnel0 + Materials[material].Fresnel1 * pow(1.0 - cosThetaAbs, 5.0);
	preflect *= materialTexData.Reflectiveness.xyz;
	// Reflection vector
	vec3 sampleDir = normalize(incidentDirection - (2.0 * cosTheta) * N);
	float phongNormalization = (materialTexData.Reflectiveness.w + 2.0) / PI_2;
	bsdf += preflect * (phongNormalization * pow(saturate(dot(sampleDir, excidentDirection)), materialTexData.Reflectiveness.w)) * cosThetaOutInv;

	// Diffuse
	vec3 prefract = 1.0 - saturate(preflect);
	vec3 pdiffuse = prefract * materialTexData.Opacity;
	prefract = prefract - pdiffuse;
	bsdf += pdiffuse * materialTexData.Diffuse / PI;

	// Refract
	float eta = cosTheta < 0.0 ? 1.0/Materials[material].RefractionIndexAvg : Materials[material].RefractionIndexAvg;
	float etaSq = eta*eta;
	float sinTheta2Sq = etaSq * (1.0f - cosTheta * cosTheta);
	// No total reflection?
	if( sinTheta2Sq < 1.0f )
	{
		// N * sign(cosTheta) = "faceForwardNormal"; -cosThetaAbs = -dot(faceForwardNormal, incidentDirection)
		sampleDir = normalize(eta * incidentDirection + (sign(cosTheta) * (sqrt(saturate(1.0 - sinTheta2Sq)) - eta * cosThetaAbs)) * N);
		if(!adjoint)
			prefract *= etaSq; // See same factor in SampleBSDF for explanation.
	}	

	// else the sampleDir is again the reflection vector
	bsdf += prefract * (phongNormalization * pow(saturate(dot(sampleDir, excidentDirection)), materialTexData.Reflectiveness.w) * cosThetaOutInv); // Potential for microoptimization by factoring out stuff from refract+reflect?

	return bsdf;
}

vec3 BSDF(vec3 incidentDirection, vec3 excidentDirection, int material, MaterialTextureData materialTexData, vec3 N)
{
	return __BSDF(incidentDirection, excidentDirection, material, materialTexData, N, false);
}

// Adjoint in the sense of Veach PhD Thesis 3.7.6
// Use this for tracing light particles, use BSDF for tracing camera paths.
vec3 AdjointBSDF(vec3 incidentDirection, vec3 excidentDirection, int material, MaterialTextureData materialTexData, vec3 N)
{
	return __BSDF(-excidentDirection, -incidentDirection, material, materialTexData, normalize(N), true);
}