// #define ANY_HIT
// #define TRACERAY_DEBUG_VARS

#ifdef ANY_HIT
	bool TraceRayAnyHit(in Ray ray, inout float rayLength)
#else
	void TraceRay(in Ray ray, inout float rayLength, out vec3 outBarycentricCoord, out Triangle outTriangle)
#endif
{
	int currentNodeIndex = 0;
	int currentLeafIndex = 0; // For highly arcane, currently unknown reasons an initial value gives a distinct performance improvement: 10ms!!
								//
	vec3 invRayDir = 1.0 / ray.Direction;
	bool nextIsLeafNode = false;

	do {
		if(!nextIsLeafNode)
		{
			// Load entire node.
			vec4 currentNode0 = texelFetch(HierachyBuffer, currentNodeIndex * 2);
			vec4 currentNode1 = texelFetch(HierachyBuffer, currentNodeIndex * 2 + 1);

			#ifdef TRACERAY_DEBUG_VARS
				++numBoxesVisited;
			#endif

			float newHit;
			//if(IntersectVirtualEllipsoid(ray, currentNode0.xyz, currentNode1.xyz, newHit) && newHit <= rayLength)
			if(IntersectBox(ray, invRayDir, currentNode0.xyz, currentNode1.xyz, newHit) && newHit <= rayLength)
			{
				uint childCode = floatBitsToUint(currentNode0.w);
				// Most significant bit tells us is this is a leaf.
				currentNodeIndex = int(childCode & uint(0x7FFFFFFF));
				nextIsLeafNode = currentNodeIndex != childCode;
				// If we go into leave mode...
				if(nextIsLeafNode)
				{
					currentLeafIndex = int(TRIANGLES_PER_LEAF * currentNodeIndex);
					currentNodeIndex = floatBitsToInt(currentNode1.w);
				}
			}
			// No hit, go to escape pointer and repeat.
			else
			{
				currentNodeIndex = floatBitsToInt(currentNode1.w);
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
				vec3 triangleNormal;
				float newHit; vec3 newBarycentricCoord;
				if(IntersectTriangle(ray, positions[0], positions[1], positions[2], newHit, newBarycentricCoord, triangleNormal) && newHit < rayLength)
				{
					#ifdef ANY_HIT
						return true;
					#else
						rayLength = newHit;
						outTriangle = triangle;
						outBarycentricCoord = newBarycentricCoord;
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