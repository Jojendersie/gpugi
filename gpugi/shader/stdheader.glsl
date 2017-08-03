#extension GL_NV_shader_atomic_float : require
#extension GL_ARB_bindless_texture : require
// Header suitable for most ray based renderers

#define RAY_HIT_EPSILON 0.0001
#define RAY_MAX 3.402823466e+38

#define RUSSIAN_ROULETTE

#include "helper.glsl"
#include "scenedata.glsl"
#include "intersectiontests.glsl"
#include "random.glsl"
#include "globaluniforms.glsl"
#include "materials.glsl"

// Debug variables for traceray.
//#define TRACERAY_DEBUG_VARS
#ifdef TRACERAY_DEBUG_VARS
	uint numBoxesVisited = 0;
	uint numTrianglesVisited = 0;
#endif

// Two versions of trace ray.
#include "traceray.glsl"
#define ANY_HIT
#include "traceray.glsl"
#undef ANY_HIT

#include "lightsource.glsl"
