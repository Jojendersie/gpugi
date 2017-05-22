layout(binding = 4, shared) uniform ImportonMapperUBO
{
	uint HashMapSize;		// Should be prime for low-collision counts
	int NumPhotonsPerLightSample;
	float HashGridSpacing;	// Must be greater or equal 2*queryradius
	float PhotonQueryRadiusSq;
};

#define HASH_MAP_BINDIDX 6
#define HASH_MAP_DATA_BINDIDX 7
#include "../maps/hashmap.glsl"