
#define INTERSECT_EPSILON 0.0001

struct Ray
{
	vec3 Origin;
	vec3 Direction;
};

bool IntersectBox(Ray ray, vec3 invRayDir, vec3 aabbMin, vec3 aabbMax, out float firstHit)
{
	vec3 tbot = invRayDir * (aabbMin - ray.Origin);
	vec3 ttop = invRayDir * (aabbMax - ray.Origin);
	vec3 tmin = min(ttop, tbot);
	vec3 tmax = max(ttop, tbot);
	vec2 t = max(tmin.xx, tmin.yz);
	firstHit = max(t.x, t.y);
	t = min(tmax.xx, tmax.yz);
	float lastHit = min(t.x, t.y);
	return firstHit <= lastHit;
}
bool IntersectBox(Ray ray, vec3 aabbMin, vec3 aabbMax, out float firstHit)
{
	return IntersectBox(ray, vec3(1.0) / ray.Direction, aabbMin, aabbMax, firstHit);
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