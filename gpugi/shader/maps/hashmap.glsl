// Implementation of a lossless hash map.
// Assumes that HashMapSize and HashGridSpacing are defined in some uniform.
// 	- uint HashMapSize;			// Should be prime for low-collision counts
//	- float HashGridSpacing;	// Must be greater or equal 2*queryradius
// Further, HASH_MAP_BINDIDX and HASH_MAP_DATA_BINDIDX must be defined before
// including this header.

#define QUADRATIC_PROBING

layout (std430, binding = HASH_MAP_BINDIDX) buffer HashMapBuffer
{
	restrict uvec2 HashMap[];
};

layout (std430, binding = HASH_MAP_DATA_BINDIDX) buffer HashMapDataBuffer
{
	// A counter to use this as an append-buffer.
	uint HashMapCounter;
	uvec3 _unused;
	// The hashmap data my be used differently by different renderers.
	// The usage can include multiple ivec4. However, the first .x value
	// of the first ivec4 is reserved for next pointers (pointing to other
	// entries in the very same buffer).
	restrict ivec4 HashMapData[];
};

ivec3 worldPosToGrid(vec3 pos)
{
	return ivec3(ceil(pos / HashGridSpacing));
}

uint gridCellHash(ivec3 cell)
{
	uint hash = (uint(cell.x) & 0x7ff)
			 | ((uint(cell.y) & 0x3ff) << 11)
			 | ((uint(cell.z) & 0x7ff) << 21);
	return RandomUInt(hash);
}

// Insert a filled data struct from HashMapData[] to the hash-grid.
// The insertion procedure is as follows:
// * Fill your data into HashMapData[] (either by deterministic indexing, or
//   appending with atomic counters)
//   Ignore the HashMapData[hmDataIndex].x value - it will be used by the hash map.
// * Call the below procedure to make sure your data is found in hash-grid
//   queries.
// This function effectively manages linked list with a head stored in the
// HashMap.
void insertToHashGrid(vec3 pos, int hmDataIndex)
{
	uint hash = gridCellHash(worldPosToGrid(pos));
	uint idx = hash % HashMapSize;
	uint probeStep = 1;
	while(true)
	{
		uint key = atomicCompSwap(HashMap[idx].x, 0xffffffff, hash);
		if(key == hash || key == 0xffffffff) // Hit an empty or the same (/linked) cell?
		{
			// Append to list front
			HashMapData[hmDataIndex].x = int(atomicExchange(HashMap[idx].y, uint(hmDataIndex)));
			break;
		} // Else: collision with some other cell
		
	#ifdef QUADRATIC_PROBING
		// Quadratic probing (h+1^2, h+2^2, h+3^2, ...)
		idx += probeStep * 2 + 1;
		probeStep ++;
	#else
		// Linear probing
		idx ++;
	#endif
		idx = idx % HashMapSize;
	}
}

// Find the first entry of a certain cell. Returns an index into the
// HashMapData array or -1 if nothing is in that cell.
// It is not certain, that the returned data really lies in that cell
// - cell hashes are repeated after 2048x1024x2048 cells.
int findFirst(ivec3 cell)
{
	uint hash = gridCellHash(cell);
	uint idx = hash % HashMapSize;
	uint key = HashMap[idx].x;
	uint probeStep = 1;
	while(key != 0xffffffff)
	{
		if(key == hash) // Hit the same (/linked) cell?
		{
			return int(HashMap[idx].y);
		} // Else: collision with some other cell
		
	#ifdef QUADRATIC_PROBING
		// Quadratic probing (h+1^2, h+2^2, h+3^2, ...)
		idx += probeStep * 2 + 1;
		probeStep ++;
	#else
		// Linear probing
		idx ++;
	#endif
		idx = idx % HashMapSize;
		
		key = HashMap[idx].x;
	}
	
	return -1;
}

// Get the next datum of a cell (see findFirst for details).
// Returns an HashMapData index or -1 at the list end.
int getNext(int hmDataIndex)
{
	return int(HashMapData[hmDataIndex].x);
}

