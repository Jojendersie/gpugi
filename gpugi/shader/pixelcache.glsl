struct PixelCacheEntry
{
	vec3 PathThroughput;	// Spectral transformation along the path back to the pixel
	uint Diffuse;			// R11G11B10 diffuse reflectance
	
	vec3 Vertex;			// The last position on the eye-path
	uint Reflectiveness;	// R11G11B10 value for the reflected percentage
	
	vec3 Incident;			// Last direction of the eye-path
	uint Opacity;			// R11G11B10 value for the transmitted percentage
	
	vec2 Normal01;
	float Shininess;		// Exponent of reflective lobes
	int MaterialID;
};

// WRITE VERSION
#ifdef SAVE_PIXEL_CACHE
layout (std430, binding=0) writeonly buffer PixelCache
{
	restrict PixelCacheEntry PixelCacheEntries[];
};

// READ VERSION
#else
layout (std430, binding=0) restrict readonly buffer PixelCache
{ 
	PixelCacheEntry PixelCacheEntries[];
};
#endif


layout(binding = 4, shared) uniform LightPathTrace
{
	int NumRaysPerLightSample;
	float LightRayPixelWeight; // (Total num pixels) / (Total num light rays) 
//	int LightCacheCapacity;
//	int AverageLightPathLength;
};