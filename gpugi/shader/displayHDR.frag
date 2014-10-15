#version 450

layout(binding = 0) uniform sampler2D Texture;

layout(location = 0) in vec2 vs_out_texcoord;
layout(location = 0, index = 0) out vec4 FragColor;

void main()
{	
	FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}