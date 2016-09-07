#define INTERSECT_EPSILON 0.0001

struct Ray
{
	vec3 Origin;
	vec3 Direction;
};

bool IntersectBox(vec3 rayOrigin, vec3 invRayDir, vec3 aabbMin, vec3 aabbMax, out float firstHit, out float lastHit)
{
	vec3 tbot = invRayDir * (aabbMin - rayOrigin);
	vec3 ttop = invRayDir * (aabbMax - rayOrigin);
	vec3 tmin = min(ttop, tbot);
	vec3 tmax = max(ttop, tbot);
	vec2 t = max(tmin.xx, tmin.yz);
	firstHit = max(0.0f, max(t.x, t.y));
	t = min(tmax.xx, tmax.yz);
	lastHit = min(t.x, t.y);
	return firstHit <= lastHit;
}
bool IntersectBox(Ray ray, vec3 aabbMin, vec3 aabbMax, out float firstHit, out float lastHit)
{
	return IntersectBox(ray.Origin, vec3(1.0) / ray.Direction, aabbMin, aabbMax, firstHit, lastHit);
}

bool IntersectVirtualEllipsoid(Ray ray, vec3 aabbMin, vec3 aabbMax, out float firstHit)
{
	// compute virual ellipsoid
	vec3 radii = (aabbMax - aabbMin) * 0.866025404 + 1e-15;
	vec3 o = ray.Origin - (aabbMax + aabbMin) * 0.5;

	// Go to sphere for numerical stability
	//float r = max(radii.x, max(radii.y, radii.z));
	float t = dot(o, ray.Direction);
	t = max(0.0f, - t );
	o += ray.Direction * t;

	o /= radii;
	float odoto = dot(o, o);

	// Test if quadratic equation for the hit point has a solution
	vec3 d = ray.Direction / radii;
	float odotd = dot(o, d);
	float ddotd = dot(d, d);
	float phalf = odotd / ddotd;
	float q = (odoto - 1.0f) / ddotd;
	float rad = phalf * phalf - q;
	if( rad < 0.0f ) return false;
	rad = sqrt(rad);
	firstHit = max(0.0f, -phalf - rad + t);
	return -phalf + rad + t >= 0.0f;
}


// Which triangle test performs well on GPU?
// Resources
// - http://gggj.ujaen.es/docs/grapp14_jimenez.pdf
// - http://ompf2.com/viewtopic.php?f=14&t=1922
// -> "Möller and Trumbore" seems to be fastest under all conditions
//
// Optix comes with two variantes: A branchless and a early exit.
// This is an adaption the branchless variant. It is basically "Möller and Trumbore" but with some clever reordering.
bool IntersectTriangle(Ray ray, vec3 p0, vec3 p1, vec3 p2,
						out float hit, out vec3 barycentricCoord, out vec3 triangleNormal)
{
	const vec3 e0 = p1 - p0;
	const vec3 e1 = p0 - p2;
	triangleNormal = cross( e1, e0 );

	const vec3 e2 = ( 1.0 / dot( triangleNormal, ray.Direction ) ) * ( p0 - ray.Origin );
	const vec3 i  = cross( ray.Direction, e2 );

	barycentricCoord.y = dot( i, e1 );
	barycentricCoord.z = dot( i, e0 );
	barycentricCoord.x = 1.0 - (barycentricCoord.z + barycentricCoord.y);
	hit   = dot( triangleNormal, e2 );

	return  /*(hit < ray.tmax) && */ (hit > INTERSECT_EPSILON) && all(greaterThanEqual(barycentricCoord, vec3(0.0)));
}

// Rotate x with a quaternion q
vec3 rotate(vec3 x, vec4 q)
{
	vec3 t = cross(q.xyz, x);
	return x + 2.0 * vec3(q.w * t.x + q.y * t.z - q.z * t.y,
						  q.w * t.y + q.z * t.x - q.x * t.z,
						  q.w * t.z + q.x * t.y - q.y * t.x);
}



// ************************************************************************* //
// The following functions use geometry IDs to fetch the data and test for the
// intersection afterwards.
// Since BVH nodes also contain the tree pointers they are fetched and
// returned too.
// ************************************************************************* //

// Fetch and intersect a box
bool FetchIntersectBoxNode(vec3 rayOrigin, vec3 invRayDir, int nodeIdx, out float firstHit, out float lastHit, out uint childCode, out int escape, out float nodeSizeSq)
{
	vec3 bbMin, bbMax;
	vec4 fetch = texelFetch(HierachyBuffer, nodeIdx * 2);
	bbMin = fetch.xyz;
	childCode = floatBitsToUint(fetch.w);
	fetch = texelFetch(HierachyBuffer, nodeIdx * 2 + 1);
	bbMax = fetch.xyz;
	escape = floatBitsToInt(fetch.w);
	nodeSizeSq = dot(bbMax-bbMin, bbMax-bbMin);
	return IntersectBox(rayOrigin, invRayDir, bbMin, bbMax, firstHit, lastHit);
}

// Fetch and intersect an oriented box
bool FetchIntersectOBoxNode(vec3 rayOrigin, vec3 rayDir, int nodeIdx, out float firstHit, out float lastHit, out uint childCode, out int escape, out float nodeSizeSq)
{
	vec3 bbCenter, bbSidesHalf;
	vec4 fetch = texelFetch(HierachyBuffer, nodeIdx * 3);
	bbCenter = fetch.xyz;
	childCode = floatBitsToUint(fetch.w);
	fetch = texelFetch(HierachyBuffer, nodeIdx * 3 + 1);
	bbSidesHalf = fetch.xyz;
	escape = floatBitsToInt(fetch.w);
	vec4 bbOrientationInv = texelFetch(HierachyBuffer, nodeIdx * 3 + 2);
	rayOrigin = rotate(rayOrigin - bbCenter, bbOrientationInv);
	rayDir = rotate(rayDir, bbOrientationInv);
	nodeSizeSq = dot(bbSidesHalf, bbSidesHalf) * 4.0;
	return IntersectBox(rayOrigin, vec3(1.0)/rayDir, -bbSidesHalf, bbSidesHalf, firstHit, lastHit);
}
