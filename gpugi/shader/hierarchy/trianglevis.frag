#version 450

#define HIT_INDEX_OUTPUT
#include "../stdheader.glsl"
#include "importanceubo.glsl"


layout(location = 0) in vec2 Texcoord;
//layout(location = 101) uniform int NumImportanceIterations;

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
	ivec2 triangleIndex = ivec2(0xFFFFFFFF);
	Triangle triangle;
	vec3 barycentricCoord;
	float rayHit = RAY_MAX;
	TraceRay(ray, rayHit, triangleIndex, barycentricCoord, triangle);
	if(rayHit != RAY_MAX)
	{
		outputImportance = HierarchyImportance[NumInnerNodes + triangleIndex.y] / NumImportanceIterations;
	}
	outputImportance = pow(outputImportance, 0.17) * 0.2;
	//outputImportance *= 0.1;

	FragColor = vec4(outputImportance);
}