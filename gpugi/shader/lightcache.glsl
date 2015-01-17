struct LightCacheEntry // aka photon
{
	vec3 Flux;
	int MaterialIndex; // -1 means "diffuse light" - TODO: Create a material for this!
	
	vec3 Position;
	float Normal0;

	vec2 Texcoord;
	float Normal1;
	float PathProbability; // "p": Probability of the underlying path.

	vec3 IncidentDirection;
	float AnyPathProbabilitySum; // "d": Propability for sampling the underlying path using any bidirectional sampling method. (if first light vertex is given)

#ifdef SHOW_ONLY_PATHLENGTH
	int PathLength;
#endif
};

// WRITE VERSION
#ifdef SAVE_LIGHT_CACHE
layout (std430, binding=0) buffer LightCache
{ 
	restrict writeonly LightCacheEntry LightCacheEntries[];
};
layout (std430, binding=1) buffer LightCacheCount
{ 
	coherent uint NumLightCacheEntries;
};


// READ VERSION
#else
layout (std430, binding=0) restrict readonly buffer LightCache
{ 
	LightCacheEntry LightCacheEntries[];
};
layout(std430, binding = 1) restrict readonly buffer LightCacheCount
{
	uint NumLightCacheEntries;
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

float MIS(float p)
{
	// No MIS - all techniques equal weighted. Should be the same as commenting out all MIS computations?
	//return 1.0;

	// Balance heuristic
	return p;

	// Power heuristic beta=2
	//return p*p;
}
