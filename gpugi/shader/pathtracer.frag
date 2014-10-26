#version 450

#include "raytrace.glsl"
#include "random.glsl"

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

void main()
{	
	uvec2 gridPosition = uvec2(vs_out_texcoord * BackbufferSize); // [0, BackbufferSize[
	uint randomSeed = InitRandomSeed(FrameSeed, gridPosition.x + gridPosition.y * BackbufferSize.x);
	vec2 screenCoord = (Random2(randomSeed) + gridPosition) / BackbufferSize * 2.0 - 1.0; // Random2 gives [0,1[. Adding [0, BackbufferSize[ should result in [0, BackbufferSize]

	Ray cameraRay;
	cameraRay.origin = CameraPosition;
	cameraRay.direction = normalize(screenCoord.x*CameraU + screenCoord.y*CameraV + CameraW);


	DefineScene();
	Intersection intersect;
	intersect.t = 1.0e+30;
	intersect.sphere = sphere[3];
	TraceRay(cameraRay, intersect);

	const vec3 lightDir = normalize(vec3(1.0, 1.0, 0.0));
	vec3 hitPos = cameraRay.origin + intersect.t * cameraRay.direction;
	vec3 color = intersect.sphere.col * dot(lightDir, normalize(hitPos - intersect.sphere.pos));

	FragColor = vec4(color, 1.0);
}