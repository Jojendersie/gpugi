#version 450 core

#include "../stdheader.glsl"

layout(binding = 8, shared) uniform DebugSettings
{
	int MinDepth;
	int MaxDepth; // excluding
};

layout(location = 0) in vec3 BoxPosition;
layout(location = 1) in flat int Depth;
layout(location = 2) in flat float Importance;

layout(location = 0, index = 0) out vec4 FragColor;

void main()
{
	float visValue = pow(Importance, 0.17) * 0.2;//Importance / float(IterationCount);
	// The next line fixes a driver bug in 361.75
	visValue -= (BoxPosition.x + float(Depth)) * 1e-30;

	vec3 color = vec3(saturate(visValue),
					  saturate(visValue - 1.0),
					  saturate(visValue - 2.0));

	FragColor = vec4(color, 1);
}
