// Implementation of a stochastic hash map which deletes other samples on collision.
// Assumes that HashMapSize and HashGridSpacing are defined in some uniform.
// 	- uint HashMapSize;			// Should be prime for low-collision counts
//	- float HashGridSpacing;	// Must be greater or equal 2*queryradius
// Further, HASH_MAP_BINDIDX and HASH_MAP_DATA_BINDIDX must be defined before
// including this header.

#include "grid.glsl"

layout (std430, binding = HASH_MAP_BINDIDX) buffer HashMapBuffer
{
	// A collision counter and data index.
	restrict ivec2 HashMap[];
};

layout (std430, binding = HASH_MAP_DATA_BINDIDX) buffer HashMapDataBuffer
{
	// A counter to use this as an append-buffer.
	uint HashMapCounter;
	uvec3 _unused;
	// The hashmap data my be used differently by different renderers.
	// The usage can use all values at will.
	restrict ivec4 HashMapData[];
};

void insertToHashGrid(vec3 pos, int hmDataIndex, inout uint seed)
{
	uint hash = gridCellHash(worldPosToGrid(pos));
	uint idx = hash % HashMapSize;
	uint n = atomicAdd(HashMap[idx].x, 1);
	// We have a collision -> make a random decision which entry will be kept
	//if(n > 0)
	{
		float decisionVar = Random(seed);
		if(decisionVar < (1.0 / (float(n)+1.0)))
			// The exchange may be invalid if someone with a greater n have
			// done an exchange before. Probably this is never the case
			// because atomicAdd and atomicExchange are likely to return in
			// the same order.
			atomicExchange(HashMap[idx].y, hmDataIndex);
	//		HashMap[idx].y = hmDataIndex;
	}
}

// Get a pair with the dataIdx (y) and the collision count (x)
ivec2 find(ivec3 cell)
{
	uint hash = gridCellHash(cell);
	uint idx = hash % HashMapSize;
	return HashMap[idx];
}
