#version 450 core

#include "../stdheader.glsl"

layout(binding = 8, shared) uniform DebugSettings
{
	int MinDepth;
	int MaxDepth; // excluding
	int IterationCount;
};

layout(location = 2) flat in float Importance;

layout(location = 0, index = 0) out vec4 FragColor;

void main()
{
	float visValue = Importance / IterationCount + 0.1f;

	vec3 color = vec3(saturate(visValue),
					  saturate(visValue - 1.0),
					  saturate(visValue - 2.0));

	FragColor = vec4(color, 1);
}