// The problem: There is no atomic vec4 operation, even with GL_NV_shader_atomic_float
// The solution: (Spin)Locks for every output pixel. (http://stackoverflow.com/questions/10652862/glsl-semaphores)
// A possible alternative solution: Gather all events in an "append buffer" and apply them via classic vertex scattering (point rendering) and blending

// coherent is necessary!
layout(binding = 0, rgba32f) coherent uniform image2D OutputTexture;
layout(binding = 1, r32ui) coherent uniform uimage2D LockTexture;

void WriteAtomic(vec3 value, ivec2 pixelCoord)
{
	// Lock pixel - there is a lot you can do wrong!
	// * ..due to shared command counter
	// * ..due to buggy glsl optimizations
	// -> Good overview: http://stackoverflow.com/questions/21538555/broken-glsl-spinlock-glsl-locks-compendium
	bool hasWritten = false; // Do not use "break;" instead. There's a bug in the GLSL optimizer!
	do {
		if(imageAtomicCompSwap(LockTexture, pixelCoord, uint(0), uint(1)) == 0)
		{
			memoryBarrierImage();

			// Add sample
			vec4 imageVal = imageLoad(OutputTexture, pixelCoord);
			imageStore(OutputTexture, pixelCoord, vec4(imageVal.xyz + value, imageVal.w));
			
			// Release Lock
			imageAtomicExchange(LockTexture, pixelCoord, 0);
			hasWritten = true;
		}
	} while(!hasWritten);
	
	//vec4 imageVal = imageLoad(OutputTexture, pixelCoord);
	//imageStore(OutputTexture, pixelCoord, vec4(imageVal.xyz + value, imageVal.w));
}