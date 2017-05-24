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
