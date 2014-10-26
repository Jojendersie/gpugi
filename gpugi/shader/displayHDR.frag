#version 450

layout(binding = 0) uniform sampler2D Texture;

layout(location = 0) in vec2 vs_out_texcoord;
layout(location = 0, index = 0) out vec4 FragColor;

void main()
{	
	FragColor = texture(Texture, vs_out_texcoord);
  FragColor.rgb /= FragColor.a;
}