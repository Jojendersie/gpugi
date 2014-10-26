#version 450

// input
layout(location = 0) in vec2 vs_in_position;

// output
layout(location = 0) out vec2 vs_out_texcoord;

void main()
{	
	gl_Position.xy = vs_in_position;
	gl_Position.zw = vec2(1.0, 1.0);
	vs_out_texcoord = vs_in_position*0.5 + 0.5;
}