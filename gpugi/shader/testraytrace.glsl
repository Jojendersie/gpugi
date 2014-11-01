struct Ray
{
	vec3 origin;
	vec3 direction;
};

struct Sphere
{
	vec3 pos;
	float radius;
	vec3 col;
};
struct Intersection
{
	float t;
	Sphere sphere;
};

bool PlaneIntersect(vec3 planeNormal, vec3 planePoint, vec3 raydirection, vec3 rayOrigin, out float t1)
{
	if(dot(planeNormal, raydirection ) > 0)
	{
		t1 = -dot(planeNormal, (rayOrigin - planePoint)) / dot(planeNormal,raydirection);
		return true;
	}
	else
		return false;
}

void SphereIntersect(const Sphere sphere, const Ray ray, inout Intersection isect)
{
	vec3 rs = ray.origin - sphere.pos;
	float B = dot(rs, ray.direction);
	float C = dot(rs, rs) - (sphere.radius * sphere.radius);
	float D = B * B - C;

	if (D > 0.0)
	{
		D = sqrt(D);
		float t0 = -B - D;
		float t1 = -B + D;
		if ( (t0 > 0.0) && (t0 < isect.t) )
		{
			isect.t = t0;
			isect.sphere = sphere;
		}
		else if ( (t1 > 0.0) && (t1 < isect.t) )
		{
			isect.t = t1;
			isect.sphere = sphere;
		}
	}
}
Sphere sphere[5];
void DefineScene()
{
	sphere[0].pos    = vec3(27, 15.5, 88.0);
	sphere[0].radius = 15.5;
	sphere[0].col = vec3(1.0, 0.1, 0.1);

	sphere[1].pos    = vec3(73, 15.5, 88.0);
	sphere[1].radius = 15.5;
	sphere[1].col = vec3(1.0, 1.0, 0.1);

	sphere[2].pos    = vec3(50.0, 15.5, 48.0);
	sphere[2].radius = 15.5;
	sphere[2].col = vec3(0.1, 0.1, 1.0);

	sphere[3].pos    = vec3(50.0, -500.0, 50.0);
	sphere[3].radius = 500.0;
	sphere[3].col = vec3(0.4);

	sphere[4].pos    = vec3(0.0);
	sphere[4].radius = 0.0;
	sphere[4].col = vec3(0.0);
}

void TraceRay(const Ray ray, inout Intersection isect)
{
	SphereIntersect(sphere[0], ray, isect);
	SphereIntersect(sphere[1], ray, isect);
	SphereIntersect(sphere[2], ray, isect);
	SphereIntersect(sphere[3], ray, isect);
}