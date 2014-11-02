//#extension GL_ARB_enhanced_layouts : require

#define NODE_TYPE_BOX 0
//#define NODE_TYPE_SPHERE 1
#define NODE_TYPE NODE_TYPE_BOX

#define TRIANGLES_PER_LEAF 8

// Theoretically GL_ARB_enhanced_layouts allows explicit memory layout.
// Reality: Such qualifiers are not allowed for structs which means that it is not possible to align arrays properly without these helper constructs.
struct PackVec3
{
	float x, y, z;
};
struct PackVec2
{
	float x, y;
};
vec3 GetVec(PackVec3 v) { return vec3(v.x, v.y, v.z); }
vec2 GetVec(PackVec2 v) { return vec2(v.x, v.y); }


struct Vertex
{
	vec3 Position;
	PackVec3 Normal;
	PackVec2 Texcoord;
};

struct Triangle
{
	uvec3 Vertices; // Indices of the vertices (array: vertices)
	uint MaterialID;
};
struct Leaf
{
	Triangle triangles[TRIANGLES_PER_LEAF];
};

#if NODE_TYPE == NODE_TYPE_BOX
struct Node
{
	vec3 BoundingBoxMin;
	uint FirstChild;	// The most significant bit determines if the child is a leaf (1)
	vec3 BoundingBoxMax;
	uint Escape;  		// Where to go next when this node was not hit? If 0 there is nowhere to go.
};
#else
	#error "No node type defined"
#endif

layout (std430, binding=0) restrict readonly buffer HierarchyBuffer 
{
	Node[] Nodes;
};
layout (std430, binding=1) restrict readonly buffer VertexBuffer 
{
	Vertex[] Vertices;
};
layout (std430, binding=2) restrict readonly buffer LeafBuffer 
{
	Leaf[] Leafs;
};