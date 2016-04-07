// #define ANY_HIT

// If defined a global variable called numBoxesVisited/numTrianglesVisited will be incremented on each box/triangle test.
// #define TRACERAY_DEBUG_VARS

// #define TRINORMAL_OUTPUT
// #define HIT_INDEX_OUTPUT
// #define TRACERAY_IMPORTANCE_BREAK

// TRINORMAL_OUTPUT: Attention! triangleNormal is not normalized

/// \param [inout] _hitIndex Node and triangle index of the final hit position. The triangle index
///		might not be defined if TRACERAY_IMPORTANCE_BREAK is enabled. The index is 0xFFFFFFFF then.
///		As input parameter this can be used to mask the intersection with a certain node or triangle
///		(e.g. that of the last hit if continuing a path). Use 0xFFFFFFFF if you do not want masking.
///
///		The AnyHit method does not change the mask.
#ifdef ANY_HIT
	bool TraceRayAnyHit(in Ray ray, in float rayLength
		#ifdef TRACERAY_IMPORTANCE_BREAK
			, in float _importanceThreshold
		#endif
	)
#else
	void TraceRay(in Ray ray, inout float rayLength
		#ifdef TRACERAY_IMPORTANCE_BREAK
			, in float _importanceThreshold
			, out float _nodeImportance
			, out float _nodeSizeSq
		#endif
		#ifdef HIT_INDEX_OUTPUT
			, out ivec2 _hitIndex
		#endif
		, out vec3 outBarycentricCoord, out Triangle outTriangle
		#ifdef TRINORMAL_OUTPUT
			, out vec3 triangleNormal
		#endif
	)
#endif
{
	int currentNodeIndex = 0;
	int currentLeafIndex = 0; // For highly arcane, currently unknown reasons an initial value gives a distinct performance improvement: 10ms!!
	#if defined(HIT_INDEX_OUTPUT) && !defined(ANY_HIT)
		_hitIndex.x = 0xFFFFFFFF;
		_hitIndex.y = 0xFFFFFFFF;
		int lastNodeIndex = 0;
	#endif
	#if defined(TRACERAY_IMPORTANCE_BREAK) && !defined(ANY_HIT)
		float lastNodeImportance = 0.0;
		float lastNodeSizeSq = 0.0;
	#endif

	vec3 invRayDir = 1.0 / ray.Direction;
	bool nextIsLeafNode = false;

	do {
		if(!nextIsLeafNode)
		{
			#ifdef TRACERAY_DEBUG_VARS
				++numBoxesVisited;
			#endif

			float newHit, exitDist, nodeSizeSq;
			uint childCode;
			int escape;
			#ifdef AABOX_BVH
			if(FetchIntersectBoxNode(ray.Origin, invRayDir, currentNodeIndex, newHit, exitDist, childCode, escape, nodeSizeSq) && newHit <= rayLength)
			#elif defined(OBOX_BVH)
			if(FetchIntersectOBoxNode(ray.Origin, ray.Direction, currentNodeIndex, newHit, exitDist, childCode, escape, nodeSizeSq) && newHit <= rayLength)
			#endif
			{
				#if defined(HIT_INDEX_OUTPUT) && !defined(ANY_HIT)
					lastNodeIndex = currentNodeIndex;
				#endif
				#ifdef TRACERAY_IMPORTANCE_BREAK
					vec2 importance = texelFetch(HierarchyImportanceBuffer, currentNodeIndex).xy;
					//float approximation = Approximation[currentNodeIndex];
					//float importance = HierarchyImportance[currentNodeIndex];
					//float importance = nodeSizeSq * 1000000.0;
					#ifndef ANY_HIT
						lastNodeImportance = importance.x;
						lastNodeSizeSq = nodeSizeSq;
					#endif
					if(((importance.x < IMPORTANCE_THRESHOLD) && (newHit > 0.0)) || (importance.y <= _importanceThreshold * newHit))
					//if((childCode & 0x80000000u) == 0x80000000u)
					{
					#ifdef ANY_HIT
						if(exitDist <= rayLength) return true;
						currentNodeIndex = escape;
					#else
						_nodeImportance = importance.x;
						_nodeSizeSq = lastNodeSizeSq;
						_hitIndex.x = currentNodeIndex;
						//rayLength = (newHit + exitDist) * 0.5;
						rayLength = newHit + (exitDist - newHit) * float(wanghash(floatBitsToUint(exitDist)) % 8388593) / 8388593.0;
						currentNodeIndex = escape;
						_hitIndex.y = 0xFFFFFFFF;
					#endif
					} else {
				#endif
				// Most significant bit tells us if this is a leaf.
				currentNodeIndex = int(childCode & uint(0x7FFFFFFF));
				nextIsLeafNode = currentNodeIndex != childCode;
				// If yes, we go into leave mode...
				if(nextIsLeafNode)
				{
					currentLeafIndex = int(TRIANGLES_PER_LEAF * currentNodeIndex);
					currentNodeIndex = escape;
				}
				#ifdef TRACERAY_IMPORTANCE_BREAK
					}
				#endif
			}
			// No hit, go to escape pointer and repeat.
			else
			{
				currentNodeIndex = escape;
			}
		}

		// If it is a leaf ...
		// The nextIsLeafNode can be changed since 'if(!nextIsLeafNode)', so do no 'else' here
		if(nextIsLeafNode)
		{
			// Load triangle.
			Triangle triangle = texelFetch(TriangleBuffer, currentLeafIndex);
			if(triangle.x == triangle.y) // Last check if this is condition helps perf: 05.11 (testscene.txt), GK104
			{
				nextIsLeafNode = false;
			} else {
				#ifdef TRACERAY_DEBUG_VARS
					++numTrianglesVisited;
				#endif

				// Load vertex positions
				vec3 positions[3];
				positions[0] = texelFetch(VertexPositionBuffer, triangle.x).xyz;
				positions[1] = texelFetch(VertexPositionBuffer, triangle.y).xyz;
				positions[2] = texelFetch(VertexPositionBuffer, triangle.z).xyz;

				// Check hit.
				vec3 newTriangleNormal;
				float newHit; vec3 newBarycentricCoord;
				if(IntersectTriangle(ray, positions[0], positions[1], positions[2], newHit, newBarycentricCoord, newTriangleNormal)
					&& newHit < rayLength)
				{
					#ifdef ANY_HIT
						return true;
					#else
						rayLength = newHit;
						outTriangle = triangle;
						outBarycentricCoord = newBarycentricCoord;
						#ifdef HIT_INDEX_OUTPUT
							_hitIndex.x = lastNodeIndex;
							_hitIndex.y = currentLeafIndex;
						#endif
						#ifdef TRACERAY_IMPORTANCE_BREAK
							_nodeImportance = lastNodeImportance;
							_nodeSizeSq = lastNodeSizeSq;
						#endif

					#ifdef TRINORMAL_OUTPUT
						triangleNormal = newTriangleNormal;
					#endif

						// Cannot return yet, there might be a triangle that is hit before this one!
					#endif
				}

				++currentLeafIndex;
				nextIsLeafNode = (currentLeafIndex % TRIANGLES_PER_LEAF) != 0;
			}
		}

	} while(currentNodeIndex != 0 || nextIsLeafNode);

#ifdef ANY_HIT
	return false;
#endif
}
