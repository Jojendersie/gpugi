#version 450

#include "raytrace.glsl"
#include "random.glsl"
#include "helper.glsl"

layout(binding = 0, std140) uniform GlobalConst
{
	uvec2 BackbufferSize;
};

layout(binding = 1, std140) uniform Camera
{
	vec3 CameraU;
	vec3 CameraV;
	vec3 CameraW;
	vec3 CameraPosition;
};

layout(binding = 2, std140) uniform PerIteration
{
	uint FrameSeed;
};

layout(location = 0) in vec2 vs_out_texcoord;
layout(location = 0, index = 0) out vec4 FragColor;

#define MAX_NUM_BOUNCES 1
#define RAY_HIT_EPSILON 0.01
//#define RUSSIAN_ROULETTE

float TraceRay(in Ray ray, out vec3 normal)
{
	// Load node
	uint currentNodeIndex = 0;
	do {
		float rayHit;
		if(IntersectBox(ray, GetVec(Nodes[currentNodeIndex].BoundingBoxMin), GetVec(Nodes[currentNodeIndex].BoundingBoxMax), rayHit))
		{
			uint parentNodeIndex = currentNodeIndex;
			uint childCode = Nodes[currentNodeIndex].FirstChild;
			currentNodeIndex = childCode & uint(0x7FFFFFFF);  // Most significant bit tells us is this is a leaf.
		
			// It is a leaf
			if(currentNodeIndex != childCode)
			{
				/*for(int i=0; i<TRIANGLES_PER_LEAF; ++i)
				{
					// Load triangle.
					Triangle triangle = Leafs[currentNodeIndex].triangles[i];
					// Load vertex positions
					vec3 positions[3];
					positions[0] = GetVec(Vertices[triangle.Vertices.x].Position);
					positions[1] = GetVec(Vertices[triangle.Vertices.y].Position);
					positions[2] = GetVec(Vertices[triangle.Vertices.z].Position);

					// Check hit.
					float beta, gamma;
					if(IntersectTriangle(ray, positions[0], positions[1], positions[2], rayHit, beta, gamma, normal))
					{
						return rayHit;
					}
				}
				currentNodeIndex = Nodes[parentNodeIndex].Escape; */

				return rayHit;
			}
		}
		// No hit, go to escape pointer and repeat.
		else
		{
			currentNodeIndex = Nodes[currentNodeIndex].Escape;
		}
	} while(currentNodeIndex != 0);

	return -1.0f;
}

void main()
{	
	uvec2 gridPosition = uvec2(vs_out_texcoord * BackbufferSize); // [0, BackbufferSize[
	uint randomSeed = InitRandomSeed(FrameSeed, gridPosition.x + gridPosition.y * BackbufferSize.x);
	vec2 screenCoord = (Random2(randomSeed) + gridPosition) / BackbufferSize * 2.0 - 1.0; // Random2 gives [0,1[. Adding [0, BackbufferSize[ should result in [0, BackbufferSize]

	Ray cameraRay;
	cameraRay.Origin = CameraPosition;
	cameraRay.Direction = normalize(screenCoord.x*CameraU + screenCoord.y*CameraV + CameraW);

	const vec3 lightDir = normalize(vec3(1.0, 1.0, 0.0));
	vec3 color = vec3(0.0);
	vec3 rayColor = vec3(1.0);

	for(int i=0; i<MAX_NUM_BOUNCES; ++i)
	{
		vec3 normal;
		float rayHit = TraceRay(cameraRay, normal);
		if(rayHit < RAY_HIT_EPSILON)
			break;

		color = vec3(rayHit*0.01); //vec3(normalize(abs(normal)));
/*
#ifdef RUSSIAN_ROULETTE
		float notAbsorption = dot(intersect.sphere.col, vec3(0.333333));
		rayColor *= intersect.sphere.col / notAbsorption; // Only change in spectrum, no energy loss.
#else
		rayColor *= intersect.sphere.col; // Absorption, not via Russion Roulette, but by color multiplication.
#endif

		
		// Add direct light - DIRECTIONAL TEST LIGHT ATM
		cameraRay.Origin = cameraRay.Origin + (intersect.t - RAY_HIT_EPSILON) * cameraRay.direction;
		vec3 hitNormal = vec3(0, 1, 0); //normalize(cameraRay.Origin - intersect.sphere.pos);
		intersect.t = 1.0e+30;
		cameraRay.Direction = lightDir;
		TraceRay(cameraRay, intersect);
		if(intersect.t >= 1.0e+30)
			color += rayColor * saturate(dot(lightDir, hitNormal)); // 1/PI is BRDF, rayColor incoming L

#ifdef RUSSIAN_ROULETTE
		if(Random(randomSeed) > notAbsorption)
			break;
#endif

		// Bounce ray.
		vec3 U, V;
		CreateONB(hitNormal, U, V);
		cameraRay.direction = SampleUnitHemisphere(Random2(randomSeed), U, V, hitNormal);
		intersect.t = 1.0e+30;
		intersect.sphere = sphere[4];

		*/
	}

	FragColor = vec4(color, 1.0);
}