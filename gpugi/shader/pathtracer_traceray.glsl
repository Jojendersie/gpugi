// #define ANY_HIT
// #define TRACERAY_DEBUG_VARS

#ifdef ANY_HIT
	bool TraceRayAnyHit(in Ray ray, inout float rayLength)
#else
	void TraceRay(in Ray ray, inout float rayLength, out vec3 outBarycentricCoord, out Triangle outTriangle)
#endif
{
	int currentNodeIndex = 0;
	vec3 invRayDir = 1.0 / ray.Direction;

	do {
		// Load entire node.
		vec4 currentNode0 = texelFetch(HierachyBuffer, currentNodeIndex * 2);
		vec4 currentNode1 = texelFetch(HierachyBuffer, currentNodeIndex * 2 + 1);

		#ifdef TRACERAY_DEBUG_VARS
			++numBoxesVisited;
		#endif

		float newHit;
		if(IntersectBox(ray, invRayDir, currentNode0.xyz, currentNode1.xyz, newHit) && newHit <= rayLength)
		{
			uint childCode = floatBitsToUint(currentNode0.w);
			currentNodeIndex = int(childCode & uint(0x7FFFFFFF));  // Most significant bit tells us is this is a leaf.
		
			// If it is a leaf ...
			if(currentNodeIndex != childCode)
			{
				for(int i=0; i<TRIANGLES_PER_LEAF; ++i)
				{
					// Load triangle.
					Triangle triangle = texelFetch(TriangleBuffer, int(TRIANGLES_PER_LEAF * currentNodeIndex + i));
					if(triangle.x == triangle.y) // Last test if optimization or perf decline: 02.11.14
						continue;
						
					#ifdef TRACERAY_DEBUG_VARS
						++numTrianglesVisited;
					#endif

					// Load vertex positions
					vec3 positions[3];
					positions[0] = texelFetch(VertexBuffer, triangle.x * 2).xyz;//Vertices[triangle.x].Position;
					positions[1] = texelFetch(VertexBuffer, triangle.y * 2).xyz;//Vertices[triangle.y].Position;
					positions[2] = texelFetch(VertexBuffer, triangle.z * 2).xyz;//Vertices[triangle.z].Position;

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
				}
				currentNodeIndex = floatBitsToInt(currentNode1.w);
			}
		}
		// No hit, go to escape pointer and repeat.
		else
		{
			currentNodeIndex = floatBitsToInt(currentNode1.w);
		}
	} while(currentNodeIndex != 0);

#ifdef ANY_HIT
	return false;
#endif
}