struct LightCacheEntry // aka photon
{
	vec3 Flux;
	int MaterialIndex; // -1 means "diffuse light" - TODO: Create a material for this!
	
	vec3 Position;
	float Normal0;

	vec2 Texcoord;
	float Normal1;
	int PathLength;

	vec3 IncidentDirection;
	int _unused;
};

// WRITE VERSION
#ifdef SAVE_LIGHT_CACHE
layout (std430, binding=0) writeonly buffer LightCache
{
	restrict LightCacheEntry LightCacheEntries[];
};
layout (std430, binding=1) buffer LightCacheCount
{ 
	coherent int NumLightCacheEntries;
};

//#ifdef SAVE_LIGHT_CACHE_WARMUP
layout(std430, binding = 2) buffer LightPathLength
{
	coherent int LightPathLengthSum;
};
//#endif


// READ VERSION
#else
layout (std430, binding=0) restrict readonly buffer LightCache
{ 
	LightCacheEntry LightCacheEntries[];
};
layout(std430, binding = 1) restrict readonly buffer LightCacheCount
{
	int NumLightCacheEntries;
};
#endif



layout(binding = 4, shared) uniform LightPathTrace
{
	int NumRaysPerLightSample;
	float LightRayPixelWeight; // (Total num pixels) / (Total num light rays) 
	int LightCacheCapacity;
	int AverageLightPathLength;
};

// Weighting function for single probabilities in MIS computations.
// Algorithms may assume that MISHeuristic(a*b) == MISHeuristic(a) * MISHeuristic(b)
float MISHeuristic(float p)
{
	// No MIS - all techniques equal weighted. Should be the same as commenting out all MIS computations.
	//return 1.0;

	// Balance heuristic
	//return min(1000, p);
	return p;

	// Power heuristic beta=2
	//return min(1000, p*p);
	//return p*p;
}

// Probability of a back projection
float InitialMIS()
{
	return 1.8;
}

// d: Distance to the next event estimate point
// c: cosine on current point (the flatter the less likely are random hits
//    prefer NEE in that case.
float DistanceMISHeuristic(float d, float c)
{
	return MISHeuristic(d*d/(c+DIVISOR_EPSILON));
	//return MISHeuristic(d*d);
	//return 1.0;
}
