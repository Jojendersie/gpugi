#version 440
#extension GL_ARB_bindless_texture : require
// Header suitable for most ray based renderers

#include "helper.glsl"
#include "scenedata.glsl"
#include "intersectiontests.glsl"
#include "random.glsl"
#include "globaluniforms.glsl"
#include "materials.glsl"

// Debug variables for traceray.
/*#define TRACERAY_DEBUG_VARS
uint numBoxesVisited = 0;
uint numTrianglesVisited = 0;*/

// Two versions of trace ray.
#include "traceray.glsl"
#define ANY_HIT
#include "traceray.glsl"
#undef ANY_HIT

#define RAY_HIT_EPSILON 0.001
#define RAY_MAX 3.40282347e+38