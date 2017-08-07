// The problem: There is no atomic vec4 operation, even with GL_NV_shader_atomic_float
// The solution: (Spin)Locks for every output pixel. (http://stackoverflow.com/questions/10652862/glsl-semaphores)
// A possible alternative solution: Gather all events in an "append buffer" and apply them via classic vertex scattering (point rendering) and blending

// coherent is necessary!
//layout(binding = 0, rgba32f) coherent uniform image2D OutputTexture;
layout(binding = 0, r32f) coherent uniform image2D OutputTexture;
layout(binding = 1, r32ui) coherent uniform uimage2D LockTexture;

#ifdef DEPTH_BUFFER_OCCLUSION_TEST
layout(binding = 2, rgba32f) coherent readonly uniform image2D PositionTexture;
#endif

//#define RECONSTRUCTION_FILTER 1 // Mitchell 1/3,1/3
//#define RECONSTRUCTION_FILTER 2 // Cone
//#define RECONSTRUCTION_FILTER 3 // BSpline (Mitchell 1,0)
// Default: box = 0

void WriteAtomic(vec3 value, ivec2 pixelCoord)
{
	// Lock pixel - there is a lot you can do wrong!
	// * ..due to shared command counter
	// * ..due to buggy glsl optimizations
	// -> Good overview: http://stackoverflow.com/questions/21538555/broken-glsl-spinlock-glsl-locks-compendium
	/*bool hasWritten = false; // Do not use "break;" instead. There's a bug in the GLSL optimizer!
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
	} while(!hasWritten);//*/
	
	// Non-atomic
	//vec4 imageVal = imageLoad(OutputTexture, pixelCoord);
	//imageStore(OutputTexture, pixelCoord, vec4(imageVal.xyz + value, imageVal.w));
	
	pixelCoord.x *= 4;
	imageAtomicAdd(OutputTexture, pixelCoord, value.x);
	imageAtomicAdd(OutputTexture, pixelCoord+ivec2(1,0), value.y);
	imageAtomicAdd(OutputTexture, pixelCoord+ivec2(2,0), value.z);
}

// Compute a screen contribution of some point and add it to the output image.
// 'ray' defines the position for back projection and the incident direction for shading
// purposes.
// computeFullBSDF: compute full BSDF (true) or diffuse part only
void ProjectToScreen(
	in Ray ray,
	in vec3 geometryNormal,
	in vec3 shadingNormal,
	in MaterialData materialData,
	in vec3 pathThroughput,
	in bool computeFullBSDF,
#ifdef CHAINED_MIS
	in float cmisQualityN,
	in float cmisQualityT,
	in float lastConnectionProbability,
	in float lastDistMIS,
#endif
	in bool computeMIS
) {
	Ray cameraRay;
	cameraRay.Direction = CameraPosition - ray.Origin;
	float camViewDotRay = -dot(CameraW, cameraRay.Direction);
#ifdef DEPTH_BUFFER_OCCLUSION_TEST
	float cosAtSurface = dot(cameraRay.Direction, shadingNormal);
	if(camViewDotRay > 0.0 /*&& cosAtSurface > 0.0*/) // Camera faces point?
#else
	if(camViewDotRay > 0.0)
#endif
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

			vec2 pixelPos = (screenCoord*0.5 + vec2(0.5)) * BackbufferSize;
			ivec2 pixelCoord = ivec2(round(pixelPos));
			
		#ifdef DEPTH_BUFFER_OCCLUSION_TEST
			//float viewDepth = imageLoad(OutputTexture, pixelCoord).w;
		/*	float viewDepth = imageAtomicAdd(OutputTexture, pixelCoord*ivec2(4,1)+ivec2(3,0), 0.0);
			float relativeBias = 1.001;//1.0 + 0.001/max(DIVISOR_EPSILON, abs(dot(cameraRay.Direction, geometryNormal)));
			if(cameraDist <= viewDepth * relativeBias)//*/
			//if(abs(cameraDist - viewDepth) / cameraDist <= 0.01)
			
			vec2 packedOccluderNormal;
			//packedOccluderNormal.x = imageLoad(OutputTexture, pixelCoord).w;
			// The following line should load a channel from r32f binding, but does
			// not. The atomic add is a work around to load the data.
			//packedOccluderNormal.x = imageLoad(OutputTexture, pixelCoord * ivec2(4,1)).x;
			packedOccluderNormal.x = imageAtomicAdd(OutputTexture, pixelCoord*ivec2(4,1)+ivec2(3,0), 0.0);
			vec4 occluderPos_Normal = imageLoad(PositionTexture, pixelCoord);
			packedOccluderNormal.y = occluderPos_Normal.w;
			vec3 occluderNormal = UnpackNormal(packedOccluderNormal);
			occluderPos_Normal.xyz -= 4 * RAY_HIT_EPSILON * occluderNormal;
			//WriteAtomic(abs(occluderNormal) * 0.1, pixelCoord);
			//WriteAtomic(vec3(packedOccluderNormal.y * 0.2), pixelCoord);
			//return;
			//if(dot(cameraRay.Origin - occluderPos_Normal.xyz, occluderNormal) >= 0.0)
			float planeDist = dot(cameraRay.Origin - occluderPos_Normal.xyz, occluderNormal);
			planeDist /= cameraDist;
			if(planeDist >= -5e-4 && planeDist < 5e-3)//*/
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
				if(computeFullBSDF)
				{
					float pdf;

					// here we have an eye-tracing segment, so we don't use the adjoint methods
					// but since we are coming from the other direction, we swap in and out direction.
					//bsdf = BSDF(-cameraRay.Direction, -ray.Direction, materialData, shadingNormal, pdf);
					bsdf = AdjointBSDF(ray.Direction, cameraRay.Direction, materialData, shadingNormal, pdf);
					
				#ifdef SAVE_LIGHT_CACHE
					if(computeMIS)
					{
						float connectionEyeToLightPath = InitialMIS(); // See lightcache.glsl initial value
						float connectionLightPathToEye = MISHeuristic(pdf);
						float mis = connectionEyeToLightPath / max(connectionLightPathToEye + connectionEyeToLightPath, DIVISOR_EPSILON);
					#ifdef CHAINED_MIS
					//	cmisQualityN = cmisQualityT = 1.0;
						// Require the PT-MIS for the last path segment too, to update the qualities.
					//	float lastPTmis = 1.0 - connectionLightPathToEye / max(connectionLightPathToEye + lastConnectionProbability * lastDistMIS, DIVISOR_EPSILON);
						float lastPTmis = lastConnectionProbability / max(lastConnectionProbability + connectionLightPathToEye * lastDistMIS, DIVISOR_EPSILON);
						updateQuality(cmisQualityN, lastPTmis);
						updateQuality(cmisQualityT, 1.0 - lastPTmis);
						
						// Change projection MIS dependent on PT MIS quality
						mis = qualityWeightedMIS_NEE(mis, cmisQualityN, cmisQualityT);
					#endif
						bsdf *= mis;
					}
				#endif
				} else {
					float surfaceInCos = dot(ray.Direction, shadingNormal);
					float surfaceOutCos = dot(cameraRay.Direction, shadingNormal);
					if(surfaceInCos * surfaceOutCos < 0.0)
					{
						vec3 pDiffuse = GetDiffuseProbability(materialData, abs(surfaceInCos));
						bsdf = pDiffuse * materialData.Diffuse * Avg(pDiffuse) / PI;
						//bsdf = GetDiffuseReflectance(materialData, abs(surfaceInCos)) / PI;
					}
					else bsdf = vec3(0.0);
				}
				vec3 color = pathThroughput * fluxToIrradiance * bsdf;
				
				// Mitchell-Netravali-Filter
			#if 1 == RECONSTRUCTION_FILTER
				for(int y = -2; y <= 2; ++y) for(int x = -2; x <= 2; ++x)
				{
					ivec2 p = ivec2(pixelCoord.x+x, pixelCoord.y+y);
					float d = length(pixelPos - vec2(p));
					float filterValue = 0.0;
					if(d < 1.0)
						filterValue = (7.0 * d*d*d - 12.0 * d*d + 16.0 / 3.0) / 6.0;
					else if(d < 2.0)
						filterValue = (-7.0/3.0 * d*d*d + 12.0 * d*d - 20.0 * d + 32.0/3.0) / 6.0;
					WriteAtomic(filterValue * color, p);
				}
			#elif 2 == RECONSTRUCTION_FILTER
				for(int y = -2; y <= 2; ++y) for(int x = -2; x <= 2; ++x)
				{
					ivec2 p = ivec2(pixelCoord.x+x, pixelCoord.y+y);
					float d = length(pixelPos - vec2(p));
					float filterValue = 0.0;
					if(d < 1.5)
						filterValue = 1.0 - d / 1.5;
					WriteAtomic(filterValue * color, p);
				}
			#elif 3 == RECONSTRUCTION_FILTER
				for(int y = -2; y <= 2; ++y) for(int x = -2; x <= 2; ++x)
				{
					ivec2 p = ivec2(pixelCoord.x+x, pixelCoord.y+y);
					float d = length(pixelPos - vec2(p));
					float filterValue = 0.0;
					if(d < 1.0)
						filterValue = (3.0 * d*d*d - 6.0 * d*d + 4.0) / 6.0;
					else if(d < 2.0)
						filterValue = (-d*d*d + 6.0 * d*d - 12.0 * d + 8.0) / 6.0;
					WriteAtomic(filterValue * color, p);
				}
			#else
				WriteAtomic(color, pixelCoord);
			#endif
			}
		}
	}
}
