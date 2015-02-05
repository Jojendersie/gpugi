#version 450 core

layout(binding = 10) uniform sampler2D RenderTexture;
layout(binding = 13) uniform sampler2D ReferenceTexture;

layout(location = 0) in vec2 Texcoord;

layout(location = 0, index = 0) out float FragColor;

layout(location = 0) uniform uint Divisor;

void main()
{
  vec4 colorRender = texture(RenderTexture, Texcoord);
  vec4 colorRef = texture(ReferenceTexture, Texcoord);
  vec3 diff = colorRender.rgb / Divisor - colorRef.rgb;
  FragColor = dot(diff, diff);
}