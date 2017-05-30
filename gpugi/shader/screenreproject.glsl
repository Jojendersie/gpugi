// The problem: There is no atomic vec4 operation, even with GL_NV_shader_atomic_float
// The solution: (Spin)Locks for every output pixel. (http://stackoverflow.com/questions/10652862/glsl-semaphores)
// A possible alternative solution: Gather all events in an "append buffer" and apply them via classic vertex scattering (point rendering) and blending

// coherent is necessary!
layout(binding = 0, rgba32f) coherent uniform image2D OutputTexture;
layout(binding = 1, r32ui) coherent uniform uimage2D LockTexture;

//#define DEPTH_BUFFER_OCCLUSION_TEST

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

// Compute a screen contribution of some point and add it to the output image.
// 'ray' defines the position for back projection and the incident direction for shading
// purposes.
void ProjectToScreen(in Ray ray, in vec3 geometryNormal, in vec3 shadingNormal, in MaterialData materialData, in vec3 pathThroughput, in bool computeBSDF)
{
	Ray cameraRay;
	cameraRay.Direction = CameraPosition - ray.Origin;
	float camViewDotRay = -dot(CameraW, cameraRay.Direction);
	float cosAtSurface = dot(cameraRay.Direction, shadingNormal);
	if(camViewDotRay > 0.0 && cosAtSurface > 0.0) // Camera faces point?
	{
		// Compute screen coord for this ray.
		vec3 proj = vec3(-dot(CameraU, cameraRay.Direction), -dot(CameraV, cameraRay.Direction), camViewDotRay);
		vec2 screenCoord = proj.xy / proj.z;
		screenCoord.x /= dot(CameraU,CameraU);
		screenCoord.y /= dot(CameraV,CameraV);

		// Valid screencoors
		if(screenCoord.x >= -1.0 && screenCoord.x < 1.0 && screenCoord.y > -1.0 && screenCoord.y <= 1.0)
		{
			float cameraDistSq = dot(cameraRay.Direction, cameraRay.Direction);
			float cameraDist = sqrt(cameraDistSq);
			cameraRay.Direction /= cameraDist;
			cameraRay.Origin = cameraRay.Direction * RAY_HIT_EPSILON + ray.Origin;

			ivec2 pixelCoord = ivec2((screenCoord*0.5 + vec2(0.5)) * BackbufferSize);
			
		#ifdef DEPTH_BUFFER_OCCLUSION_TEST
			float viewDepth = imageLoad(OutputTexture, pixelCoord).w;
			if(cameraDist <= viewDepth * 1.001)
			//if(abs(cameraDist - viewDepth) / cameraDist <= 0.01)
		#else
			// Hits *pinhole* camera
			if(!TraceRayAnyHit(cameraRay, cameraDist))
		#endif
			{
				// Compute pdf conversion factor from image plane area to surface area.
				// The problem: A pixel sees more than only a point! -> photometric distance law (E = I cosTheta / d²) not directly applicable!
				// -> We need the solid angle and the projected area of the pixel in this point!
				// Formulas explained in (6.2) http://www.maw.dk/3d_graphics_projects/downloads/Bidirectional%20Path%20Tracing%202009.pdf
				// Also a look into SmallVCM is a good idea!

				// The distance to the image plane  is length(CameraW) == 1.
				// imagePointToCameraDist = length(pixelPos) = length(CameraU * screenCoord.x + CameraV * screenCoord.y + CameraW) =
				//					      = 1.0 / dot(-cameraRay.Direction, CameraW)
				// fluxToIntensity = 1/solidAngle = imagePointToCameraDist / (dot(-cameraRay.Direction, CameraW) * PixelArea) =
				//				   =  1.0 / (dot(-cameraRay.Direction, CameraW)² * PixelArea)  ------ THIS IS APPROXIMATE!
				// fluxToIrradiance =(photometric distance law)= fluxToIntensity * dot(cameraRay.Direction, Normal) / cameraDistSq
					
				float cosAtCamera = saturate(camViewDotRay / cameraDist); // = dot(-cameraRay.Direction, CameraW);
				float fluxToIrradiance = AdjointBSDFShadingNormalCorrectedOutDotN(ray.Direction, cameraRay.Direction, geometryNormal, shadingNormal) /
					(cosAtCamera * cosAtCamera * cosAtCamera * PixelArea * cameraDistSq);

				vec3 bsdf = vec3(1.0);
				if(computeBSDF)
				{
					float pdf;

					// here we have an eye-tracing segment, so we don't use the adjoint methods
					// but since we are coming from the other direction, we swap in and out direction.
					bsdf = BSDF(-cameraRay.Direction, -ray.Direction, materialData, shadingNormal, pdf);
					//bsdf = AdjointBSDF(ray.Direction, cameraRay.Direction, materialData, shadingNormal, pdf);
				}
				//vec3 bsdf = GetDiffuseReflectance(materialData, cosThetaAbs) / PI;
				vec3 color = pathThroughput * fluxToIrradiance * bsdf;

				// Add Sample.
				//ivec2 pixelCoord = ivec2((screenCoord*0.5 + vec2(0.5)) * BackbufferSize);

				WriteAtomic(color, pixelCoord);
			}
		}
	}
}
