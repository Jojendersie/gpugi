// Chained MIS needs to track all previous mis values in PT.
// In light tracing this is hopefully optimized away.
float g_mis[MAX_PATHLENGTH];

void updateQuality(inout float prevQuality, float mis)
{
	//prevQuality = prevQuality * mis;
	prevQuality = prevQuality * (mis + 1.0);
}

float qualityWeightedMIS_NEE(float mis_nee, float quality_nee, float quality_trace)
{
	return mis_nee * quality_nee / max(mis_nee * quality_nee + (1-mis_nee) * quality_trace, DIVISOR_EPSILON);
}

float qualityWeightedMIS_Trace(float mis_trace, float quality_nee, float quality_trace)
{
	return mis_trace * quality_trace / max((1-mis_trace) * quality_nee + mis_trace * quality_trace, DIVISOR_EPSILON);
}

float cmisPathThroughputWeight(int pathSegment, float quality_nee, float quality_trace)
{
	// Special treatment for the first path segment, because it decides between PT and LT
	// and has nothing to do with our usual NEE.
	float w = pathSegment > 0 ? g_mis[0] : 1.0;
	w = 1.0;
	for(int i = pathSegment-1; i >= 0; --i)
	{
		w *= qualityWeightedMIS_Trace(g_mis[i], quality_nee, quality_trace);
		updateQuality(quality_nee, g_mis[i]);
		updateQuality(quality_trace, 1.0 - g_mis[i]);
	}
	return w;
}
