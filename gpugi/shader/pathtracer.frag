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
#define RAY_MAX 3.40282347e+38
//#define RUSSIAN_ROULETTE

void TraceRay(in Ray ray, inout float rayLength, out vec3 outBarycentricCoord, out Triangle outTriangle)
{
	uint currentNodeIndex = 0;

	vec3 invRayDir = 1.0 / ray.Direction;

	do {
		// Load entire node.
		Node currentNode = Nodes[currentNodeIndex];

		float newHit;
		if(IntersectBox(ray, invRayDir, currentNode.BoundingBoxMin, currentNode.BoundingBoxMax, newHit) && newHit <= rayLength)
		{
			uint childCode = currentNode.FirstChild;
			currentNodeIndex = childCode & uint(0x7FFFFFFF);  // Most significant bit tells us is this is a leaf.
		
			// It is a leaf
			if(currentNodeIndex != childCode)
			{
				for(int i=0; i<TRIANGLES_PER_LEAF; ++i)
				{
					// Load triangle.
					Triangle triangle = Leafs[currentNodeIndex].triangles[i];
					if(triangle.Vertices.x == triangle.Vertices.y) // Last test if optimization or perf decline: 02.11.14
						continue;

					// Load vertex positions
					vec3 positions[3];
					positions[0] = Vertices[triangle.Vertices.x].Position;
					positions[1] = Vertices[triangle.Vertices.y].Position;
					positions[2] = Vertices[triangle.Vertices.z].Position;

					// Check hit.
					vec3 triangleNormal;
					float newHit; vec3 newBarycentricCoord;
					if(IntersectTriangle(ray, positions[0], positions[1], positions[2], newHit, newBarycentricCoord, triangleNormal) && newHit < rayLength)
					{
						rayLength = newHit;
						outTriangle = triangle;
						outBarycentricCoord = newBarycentricCoord;
						// Cannot return yet, there might be a triangle that is hit before this one!
					}
					
				}
				currentNodeIndex = currentNode.Escape;
			}
		}
		// No hit, go to escape pointer and repeat.
		else
		{
			currentNodeIndex = currentNode.Escape;
		}
	} while(currentNodeIndex != 0);
}

void main()
{	
	uvec2 gridPosition = uvec2(vs_out_texcoord * BackbufferSize); // [0, BackbufferSize[
	uint randomSeed = InitRandomSeed(FrameSeed, gridPosition.x + gridPosition.y * BackbufferSize.x);
	vec2 screenCoord = (Random2(randomSeed) + gridPosition) / BackbufferSize * 2.0 - 1.0; // Random2 gives [0,1[. Adding [0, BackbufferSize[ should result in [0, BackbufferSize]

	Ray ray;
	ray.Origin = CameraPosition;
	ray.Direction = normalize(screenCoord.x*CameraU + screenCoord.y*CameraV + CameraW);

	vec3 color = vec3(0.0);
	vec3 rayColor = vec3(1.0);

	for(int i=0; i<MAX_NUM_BOUNCES; ++i)
	{
		// Trace ray.
		Triangle triangle;
		vec3 barycentricCoord;
		float rayHit = RAY_MAX;
		TraceRay(ray, rayHit, barycentricCoord, triangle);
		if(rayHit == RAY_MAX)
			break;

		// Compute hit normal.
		vec3 hitNormal = normalize(GetVec(Vertices[triangle.Vertices.x].Normal) * barycentricCoord.x + 
						 		   GetVec(Vertices[triangle.Vertices.y].Normal) * barycentricCoord.y +
								   GetVec(Vertices[triangle.Vertices.z].Normal) * barycentricCoord.z); 

		// Compute hit color.
		vec3 hitColor = vec3(0.7); // TODO
#ifdef RUSSIAN_ROULETTE
		float hitLuminance = GetLuminance(hitColor);
		rayColor *= hitColor / hitLuminance; // Only change in spectrum, no energy loss.
#else
		rayColor *= hitColor; // Absorption, not via Russion Roulette, but by color multiplication.
#endif


		// Add direct light - FIXED POINT LIGHT ATM
		const vec3 lightPos = vec3(0, 2, 0);
		ray.Origin = ray.Origin + (rayHit - RAY_HIT_EPSILON) * ray.Direction;
		ray.Direction = lightPos - ray.Origin;

		float lightDist = length(ray.Direction);
		ray.Direction /= lightDist;
		rayHit = lightDist;

		TraceRay(ray, rayHit, barycentricCoord, triangle);
		if(rayHit == lightDist)
			color += rayColor * saturate(dot(ray.Direction, hitNormal)); // 1/PI is BRDF, rayColor incoming L

#ifdef RUSSIAN_ROULETTE
		if(Random(randomSeed) > hitLuminance)
			break;
#endif

		// Bounce ray.
		vec3 U, V;
		CreateONB(hitNormal, U, V);
		ray.Direction = SampleUnitHemisphere(Random2(randomSeed), U, V, hitNormal);
	}

	FragColor = vec4(color, 1.0);
}