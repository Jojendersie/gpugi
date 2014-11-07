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
	uint NumLightSamples;
};

layout(binding = 3, std140) uniform UMaterials
{
	// Minimal assererted UBO size: 16KB
	// Material size 64 Byte -> up to 256 materials.
	Material Materials[256];
};