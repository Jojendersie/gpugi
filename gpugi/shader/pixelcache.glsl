struct PixelCacheEntry
{
	vec3 PathThroughput;	// Spectral transformation along the path back to the pixel
	float Barycentric0;		// α: barycentric coordinate for vertex 0
	
	Triangle triangle;		// The 3 vertex-indices and the material index
	
//	uint Diffuse;			// R11G11B10 diffuse reflectance
	
//	vec3 Vertex;			// The last position on the eye-path
//	uint Reflectiveness;	// R11G11B10 value for the reflected percentage
	
	vec2 Incident01;		// Last direction of the eye-path (compressed)
	float Barycentric1;		// β: barycentric coordinate for vertex 0
	bool DirectlyVisible;	// Path to this cache has lenght 1
//	uint Opacity;			// R11G11B10 value for the transmitted percentage
	
//	vec2 Normal01;
//	float Shininess;		// Exponent of reflective lobes
//	int MaterialID;
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
	float LightRayPixelWeight; // (Total num pixels) / (2 pi) 
//	int LightCacheCapacity;
//	int AverageLightPathLength;
};