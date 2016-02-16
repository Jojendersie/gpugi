#version 450 core

#define HIT_INDEX_OUTPUT
#include "../stdheader.glsl"

layout(binding = 8, shared) uniform DebugSettings
{
	int DebugType;
};

layout(location = 0) in vec2 Texcoord;

layout(location = 0, index = 0) out vec4 FragColor;

void main()
{
	vec2 screenCoord = Texcoord * 2.0 - vec2(1.0);

	Ray ray;
	ray.Origin = CameraPosition;
	ray.Direction = normalize(screenCoord.x*CameraU + screenCoord.y*CameraV + CameraW);

	vec3 color = vec3(0.0);
	vec3 pathThroughput = vec3(1.0);

	// Trace ray.
	Triangle triangle;
	vec3 barycentricCoord;
	ivec2 hitIndex;
	float rayHit = RAY_MAX;
	TraceRay(ray, rayHit, hitIndex, barycentricCoord, triangle);
	if(rayHit == RAY_MAX)
	{
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else
	{
		// Compute hit normal and texture coordinate.
		vec3 hitNormal; vec2 hitTexcoord;
		GetTriangleHitInfo(triangle, barycentricCoord, hitNormal, hitTexcoord);

		// Get Material infos.
		MaterialData materialData = SampleMaterialData(triangle.w, hitTexcoord);

		// Output debug color
		if(DebugType == 0)
			FragColor.rgb = hitNormal * 0.5 + 0.5;
		else if(DebugType == 1)
			FragColor.rgb = materialData.Diffuse;
		else if(DebugType == 2)
			FragColor.rgb = materialData.Opacity;
		else if(DebugType == 3)
			FragColor.rgb = materialData.Reflectiveness.xyz;
		else if(DebugType == 4)
			FragColor.rgb = vec3((float(hitIndex.y & 0xff) / 255.0).xx, float((hitIndex.y / 256) & 0xff) / 255.0);


		FragColor.a = 1.0;
	}
}
