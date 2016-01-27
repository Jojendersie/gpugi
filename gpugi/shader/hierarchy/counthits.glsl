void TraceRayCountHits(in Ray _ray, in float _rayLength, in float _pathImportance)
{
	int currentNodeIndex = 0;
	int currentLeafIndex = 0;
	vec3 invRayDir = 1.0 / _ray.Direction;
	bool nextIsLeafNode = false;
	
	do {
		if(!nextIsLeafNode)
		{
			// Load entire node.
			vec4 currentNode0 = texelFetch(HierachyBuffer, currentNodeIndex * 2);
			vec4 currentNode1 = texelFetch(HierachyBuffer, currentNodeIndex * 2 + 1);

			float newHit, exitDist;
			if(IntersectBox(_ray, invRayDir, currentNode0.xyz, currentNode1.xyz, newHit, exitDist)
				&& newHit <= _rayLength)
			{
				// Count up the rays passing this node
				atomicAdd(HierarchyImportance[currentNodeIndex], _pathImportance);
				
				uint childCode = floatBitsToUint(currentNode0.w);
				// Most significant bit tells us if this is a leaf.
				currentNodeIndex = int(childCode & uint(0x7FFFFFFF));
				nextIsLeafNode = currentNodeIndex != childCode;
				// If yes, we go into leave mode...
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
