// Each of the uvec2 represents four halfs
struct HierarchyMaterial
{
	uvec2 Reflectiveness;
	uvec2 Opacity;
	uvec2 Diffuse;
	uvec2 Emissivity;
	uvec2 Fresnel0_RefrIndex;
	uvec2 Fresnel1;
};

layout(std430, binding = 0) restrict buffer HierarchyMaterialBuffer
{
	HierarchyMaterial hierarchyMaterial[];
};