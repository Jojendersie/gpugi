// #define ANY_HIT
// #define TRACERAY_DEBUG_VARS

#ifdef ANY_HIT
	bool TraceRayAnyHit(in Ray ray, inout float rayLength)
#else
	void TraceRay(in Ray ray, inout float rayLength, out vec3 outBarycentricCoord, out Triangle outTriangle)
#endif
{
	uint currentNodeIndex = 0;
	vec3 invRayDir = 1.0 / ray.Direction;

	do {
		// Load entire node.
		Node currentNode = Nodes[currentNodeIndex];

		#ifdef TRACERAY_DEBUG_VARS
			++numBoxesVisited;
		#endif

		float newHit;
		if(IntersectBox(ray, invRayDir, currentNode.BoundingBoxMin, currentNode.BoundingBoxMax, newHit) && newHit <= rayLength)
		{
			uint childCode = currentNode.FirstChild;
			currentNodeIndex = childCode & uint(0x7FFFFFFF);  // Most significant bit tells us is this is a leaf.
		
			// If it is a leaf ...
			if(currentNodeIndex != childCode)
			{
				for(int i=0; i<TRIANGLES_PER_LEAF; ++i)
				{
					// Load triangle.
					Triangle triangle = Leafs[currentNodeIndex].triangles[i];
					if(triangle.Vertices.x == triangle.Vertices.y) // Last test if optimization or perf decline: 02.11.14
						continue;
						
					#ifdef TRACERAY_DEBUG_VARS
						++numTrianglesVisited;
					#endif

					// Load vertex positions
					vec3 positions[3];
					positions[0] = Vertices[triangle.Vertices.x].Position;
					positions[1] = Vertices[triangle.Vertices.y].Position;
					positions[2] = Vertices[triangle.Vertices.z].Position;

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
				currentNodeIndex = currentNode.Escape;
			}
		}
		// No hit, go to escape pointer and repeat.
		else
		{
			currentNodeIndex = currentNode.Escape;
		}
	} while(currentNodeIndex != 0);

#ifdef ANY_HIT
	return false;
#endif
}