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


layout(binding=1) uniform isamplerBuffer TriangleBuffer;
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


layout(binding=2) uniform samplerBuffer VertexPositionBuffer;

layout(binding=3) uniform samplerBuffer VertexInfoBuffer;

layout(binding=4) uniform samplerBuffer HierachyBuffer;
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

layout(binding=5) uniform samplerBuffer InitialLightSampleBuffer;
/*struct LightSample
{
	ei::Vec3 position;
	float normalPhi;			// atan2(normal.y, normal.x)
	ei::Vec3 luminance;
	float normalThetaCos;	// normal.z
};*/

#ifdef TRACERAY_IMPORTANCE_BREAK
// Importance buffer must be bound in normal scene setup for traceray.
// Including it in traceray causes the buffer to appear twice.
/*layout(std430, binding = 0) restrict readonly buffer HierarchyImportanceBuffer
{
	float HierarchyImportance[];
};*/
layout(binding=6) uniform samplerBuffer HierarchyImportanceBuffer;
#endif

#ifdef CACHE_DIRECT_DIFFUSE
layout(std430, binding = 9) restrict volatile buffer DiffuseLightCacheBuffer
{
	vec4 DiffuseLightCache[];
};
#endif

struct Material
{
	uvec2 diffuseTexHandle;
	uvec2 opacityTexHandle;
	uvec2 reflectivenessTexHandle;
	uvec2 emissivityTexHandle;
	vec3 Fresnel0;	// Fist precomputed coefficient for fresnel approximation (rgb)
	float RefractionIndexAvg;
	vec3 Fresnel1;	// Second precomputed coefficient for fresnel approximation (rgb)
	float padding;
};


void GetTriangleHitInfo(Triangle triangle, vec3 barycentricCoord, out vec3 normal, out vec2 texcoord)
{
	vec4 vdata0 = texelFetch(VertexInfoBuffer, triangle.x);
	vec4 vdata1 = texelFetch(VertexInfoBuffer, triangle.y);
	vec4 vdata2 = texelFetch(VertexInfoBuffer, triangle.z);
	normal = normalize(UnpackNormal(vdata0.xy) * barycentricCoord.x +
					   UnpackNormal(vdata1.xy) * barycentricCoord.y +
					   UnpackNormal(vdata2.xy) * barycentricCoord.z);
	texcoord = vdata0.zw * barycentricCoord.x +
				vdata1.zw * barycentricCoord.y +
				vdata2.zw * barycentricCoord.z;
}
