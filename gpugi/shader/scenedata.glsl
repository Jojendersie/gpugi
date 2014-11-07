//#extension GL_ARB_enhanced_layouts : require
#extension GL_ARB_bindless_texture : require

#define NODE_TYPE_BOX 0
//#define NODE_TYPE_SPHERE 1
#define NODE_TYPE NODE_TYPE_BOX

#define TRIANGLES_PER_LEAF 8

// Theoretically GL_ARB_enhanced_layouts allows explicit memory layout.
// Reality: Such qualifiers are not allowed for structs which means that it is not possible to align arrays properly without these helper constructs.
/*struct PackVec3
{
	float x, y, z;
};
struct PackVec2
{
	float x, y;
};
vec3 GetVec(PackVec3 v) { return vec3(v.x, v.y, v.z); }
vec2 GetVec(PackVec2 v) { return vec2(v.x, v.y); } */


layout(binding=0) uniform isamplerBuffer TriangleBuffer;
#define Triangle ivec4
/*struct Triangle
{
	uvec3 Vertices; // Indices of the vertices (array: vertices)
	uint MaterialID;
};
struct Leaf
{
	Triangle triangles[TRIANGLES_PER_LEAF];
};*/


layout(binding=1) uniform samplerBuffer VertexBuffer;
/*struct Vertex
{
	vec3 Position;
	PackVec3 Normal;
	PackVec2 Texcoord;
};*/

layout(binding=2) uniform samplerBuffer HierachyBuffer;
/*#if NODE_TYPE == NODE_TYPE_BOX
struct Node
{
	vec3 BoundingBoxMin;
	uint FirstChild;	// The most significant bit determines if the child is a leaf (1)
	vec3 BoundingBoxMax;
	uint Escape;  		// Where to go next when this node was not hit? If 0 there is nowhere to go.
};
#else
	#error "No node type defined"
#endif */

layout(binding=3) uniform samplerBuffer LightSampleBuffer;
/*
vec4 -> rgb: position, a: shared exponent color
*/

struct Material
{
	uvec2 diffuseTexHandle;
	uvec2 opacityTexHandle;
	uvec2 reflectivenessTexHandle;
	vec2 emissivityRG;
	vec3 Fresnel0;	// Fist precomputed coefficient for fresnel approximation (rgb)
	float RefractionIndexAvg;
	vec3 Fresnel1;	// Second precomputed coefficient for fresnel approximation (rgb)
	float emissivityB;
};
