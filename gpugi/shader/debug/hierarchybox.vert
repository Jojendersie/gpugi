#version 450 core

#include "../stdheader.glsl"

// SHOW_NODE_IMPORTANCE

layout(location = 0) in vec3 inPosition;
layout(location = 1) in int inBoxInstance;
layout(location = 2) in int inDepth;


layout(location = 0) out vec3 BoxPosition;
layout(location = 1) out flat int Depth;

#ifdef SHOW_NODE_IMPORTANCE
	layout(binding = 6) uniform samplerBuffer HierarchyImportanceBuffer;
	layout(binding = 10, std430) restrict buffer SubtreeImportanceBuffer
	{
		float SubtreeImportance[];
	};

	layout(location = 2) out flat float Importance;
#endif

void main()
{
	const vec3 maxs[2] = { vec3(1, 1, 1), vec3(3, 3, 3) };
	const vec3 mins[2] = { vec3(0, 0, 0), vec3(2, 2, 2) };

	vec3 worldPosition;

	#ifdef AABOX_BVH
		vec4 node0 = texelFetch(HierachyBuffer, int(inBoxInstance * 2));
		vec4 node1 = texelFetch(HierachyBuffer, int(inBoxInstance * 2 + 1));

		worldPosition.x = mix(node0.x, node1.x, inPosition.x);
		worldPosition.y = mix(node0.y, node1.y, inPosition.y);
		worldPosition.z = mix(node0.z, node1.z, inPosition.z);
	#elif defined(OBOX_BVH)
		vec4 node0 = texelFetch(HierachyBuffer, int(inBoxInstance * 3));
		vec4 node1 = texelFetch(HierachyBuffer, int(inBoxInstance * 3 + 1));
		vec4 node2 = texelFetch(HierachyBuffer, int(inBoxInstance * 3 + 2));

		worldPosition.x = mix(-node1.x, node1.x, inPosition.x);
		worldPosition.y = mix(-node1.y, node1.y, inPosition.y);
		worldPosition.z = mix(-node1.z, node1.z, inPosition.z);
		worldPosition = rotate(worldPosition, vec4(-node2.xyz, node2.w));
		worldPosition += node0.xyz;
	#endif

	gl_Position = vec4(worldPosition, 1.0) * ViewProjection;
	BoxPosition = inPosition;
	Depth = inDepth;

	#ifdef SHOW_NODE_IMPORTANCE
		Importance = texelFetch(HierarchyImportanceBuffer, inBoxInstance).x;
	#endif
}
