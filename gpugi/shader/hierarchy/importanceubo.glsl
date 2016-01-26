layout(binding = 4, std140) uniform HierarchyImportanceUBO
{
	int NumInnerNodes;
	int NumTriangles;
	int NumImportanceIterations;
};