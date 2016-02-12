#version 450 core

#include "../stdheader.glsl"

layout(binding = 8, shared) uniform DebugSettings
{
	int MinDepth;
	int MaxDepth; // excluding
};


layout(location = 0) in vec3 BoxPosition;
layout(location = 1) flat in int Depth;

layout(location = 0, index = 0) out vec4 FragColor;


void main()
{
	float relativeDepth = float(Depth - MinDepth) / (MaxDepth - MinDepth);
	
	// Use a simplistic offset heatmap scala
	relativeDepth *= 3.0; // Range should now roughly 0-3
	relativeDepth += 0.4; // Offset as "minimum color"
	vec3 depthColor = vec3(saturate(relativeDepth),
							saturate(relativeDepth - 1.0),
							saturate(relativeDepth - 2.0));

	// Edges a bit brigther
	vec3 edgeDist = abs(BoxPosition * 2.0 - 1.0);
	edgeDist = cos(edgeDist * 3.14) * 0.5 + 0.5;
	float edgeFactor = 1.0 - max(max(edgeDist.x*edgeDist.y, edgeDist.x * edgeDist.z), edgeDist.y * edgeDist.z);
	edgeFactor = pow(edgeFactor, 8);

	FragColor = vec4(vec3(clamp(edgeFactor, 0.6, 1.0)) * depthColor, 1);
}