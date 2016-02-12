void TraceRayCountHits(in Ray _ray, in float _rayLength, in float _pathImportance)
{
	int currentNodeIndex = 0;
	int currentLeafIndex = 0;
	vec3 invRayDir = 1.0 / _ray.Direction;
	bool nextIsLeafNode = false;
	
	do {
		if(!nextIsLeafNode)
		{
			float newHit, exitDist, nodeSizeSq;
			uint childCode;
			int escape;
			#ifdef AABOX_BVH
			if(FetchIntersectBoxNode(_ray.Origin, invRayDir, currentNodeIndex, newHit, exitDist, childCode, escape, nodeSizeSq) && newHit <= _rayLength)
			#elif defined(OBOX_BVH)
			if(FetchIntersectOBoxNode(_ray.Origin, _ray.Direction, currentNodeIndex, newHit, exitDist, childCode, escape, nodeSizeSq) && newHit <= _rayLength)
			#endif
			{
				// Count up the rays passing this node
				atomicAdd(HierarchyImportance[currentNodeIndex], _pathImportance);
				
				// Most significant bit tells us if this is a leaf.
				currentNodeIndex = int(childCode & uint(0x7FFFFFFF));
				nextIsLeafNode = currentNodeIndex != childCode;
				// If yes, we go into leave mode...
				if(nextIsLeafNode)
				{
					currentLeafIndex = int(TRIANGLES_PER_LEAF * currentNodeIndex);
					currentNodeIndex = escape;
				}
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
				// Load vertex positions
				vec3 positions[3];
				positions[0] = texelFetch(VertexPositionBuffer, triangle.x).xyz;
				positions[1] = texelFetch(VertexPositionBuffer, triangle.y).xyz;
				positions[2] = texelFetch(VertexPositionBuffer, triangle.z).xyz;

				// Check hit.
				vec3 newTriangleNormal;
				float newHit; vec3 newBarycentricCoord;
				if(IntersectTriangle(_ray, positions[0], positions[1], positions[2], newHit, newBarycentricCoord, newTriangleNormal)
					&& newHit <= _rayLength)
				{
					atomicAdd(HierarchyImportance[NumInnerNodes + currentLeafIndex], _pathImportance);
				}

				++currentLeafIndex;
				nextIsLeafNode = (currentLeafIndex % TRIANGLES_PER_LEAF) != 0;
			}
		}
		
	} while(currentNodeIndex != 0 || nextIsLeafNode);
}
