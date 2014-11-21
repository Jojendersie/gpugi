struct LightCacheEntry // aka photon
{
	vec3 Intensity; // Flux / PI_2
	int MaterialIndex; // -1 means "diffuse light" - TODO: Create a material for this!
	
	vec3 Position;
	float Normal0;

	vec2 Texcoord;
	float Normal1;
	float padding;

	vec3 IncidentDirection;

	// TODO:
	// - Normal (compressed?)
	// - previous entry
	// - recursive MIS weight
	// - ...?!
};

// WRITE VERSION
#ifdef SAVE_LIGHT_CACHE
layout (std430, binding=0) buffer LightCache
{ 
	coherent uint NumLightCacheEntries;
	restrict writeonly LightCacheEntry LightCacheEntries[];
};

// READ VERSION
#else
layout (std430, binding=0) restrict readonly buffer LightCache
{ 
	uint NumLightCacheEntries;
	LightCacheEntry LightCacheEntries[];
};
#endif



layout(binding = 4, shared) uniform LightPathTrace
{
	int NumRaysPerLightSample;
	float LightRayPixelWeight; // (Total num pixels) / (Total num light rays) 
	uint LightCacheCapacity;
};

#define NUM_CAMPATH_LIGHTSAMPLE_CONNECTIONS 4 // should be "average light path length num"
#define NUM_SAMPLING_TECHNIQUES NUM_CAMPATH_LIGHTSAMPLE_CONNECTIONS+1