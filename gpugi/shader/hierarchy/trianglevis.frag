#version 450

#define TRIINDEX_OUTPUT
#include "../stdheader.glsl"
#include "importanceubo.glsl"


layout(location = 0) in vec2 Texcoord;

layout(binding = 10, std430) restrict readonly buffer HierarchyImportanceBuffer
{
	float HierarchyImportance[];
};

layout(location = 0, index = 0) out vec4 FragColor;

void main()
{
	//uint randomSeed = InitRandomSeed(FrameSeed, gridPosition.x + gridPosition.y * BackbufferSize.x);
	vec2 screenCoord = Texcoord * 2.0 - vec2(1.0);

	Ray ray;
	ray.Origin = CameraPosition;
	ray.Direction = normalize(screenCoord.x*CameraU + screenCoord.y*CameraV + CameraW);

	float outputImportance = 0.0;

	// Trace ray.
	int triangleIndex;
	Triangle triangle;
	vec3 barycentricCoord;
	float rayHit = RAY_MAX;
	TraceRay(ray, rayHit, barycentricCoord, triangle, triangleIndex);
	if(rayHit != RAY_MAX)
	{
		outputImportance = HierarchyImportance[NumInnerNodes + triangleIndex] / IterationCount;
	}
	outputImportance = pow(outputImportance, 0.17) * 0.2;
	//outputImportance *= 0.1;

	FragColor = vec4(outputImportance);
}