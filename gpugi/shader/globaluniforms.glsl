layout(binding = 0, shared) uniform GlobalConst
{
	ivec2 BackbufferSize;
	int NumInitialLightSamples;
};

layout(binding = 1, shared) uniform Camera
{
	vec3 CameraU;
	vec3 CameraV;
	vec3 CameraW;
	vec3 CameraPosition;
	float PixelArea;
	mat4 ViewProjection;
};

layout(binding = 2, shared) uniform PerIteration
{
	uint FrameSeed;
	uint IterationCount;
};

layout(binding = 3, shared) uniform UMaterials
{
	// Minimal assured UBO size: 16KB
	// Material size 64 Byte -> up to 256 materials.
	Material Materials[256];
};