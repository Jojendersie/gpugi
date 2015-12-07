
// Sample a random normal from an ellipsoid seen from a certain direction
vec3 sampleNormal(vec3 _incident, mat3x3 _sggx, inout uint seed)
{
	// Compute an orthonormal basis around _incident
	mat3x3 local;
	local[0] = _incident;
	CreateONB(_incident, local[1], local[2]);
	
	// Transform _sggx into the local system????
	float Sxx = dot(local[0], _sggx * local[0]);
	float Sxy = dot(local[0], _sggx * local[1]);
	float Sxz = dot(local[0], _sggx * local[2]);
	float Syy = dot(local[1], _sggx * local[1]);
	float Syz = dot(local[1], _sggx * local[2]);
	float Szz = dot(local[2], _sggx * local[2]);
	
	// Compute basis from sphere to NDF (Cholesky decomposition)
	float det = Sxx * (Syy*Szz - Syz) + Sxy * (Sxz*Syz - Sxy*Szz) + Sxz * (Sxy*Syz - Syy*Sxz);
	float tmp = Syy*Sxx - Sxy*Sxy;
	vec3 m_i = vec3(sqrt(det / tmp), 0.0, 0.0);
	tmp = sqrt(tmp);
	vec3 m_j = vec3((-Sxz*Sxy-Syz*Sxx) / tmp, tmp, 0.0) / sqrt(Sxx);
	vec3 m_k = vec3(Sxz, Sxy, Sxx) / sqrt(Sxx);
	
	// Generate random point on visible hemisphere
	float chi0 = Random(seed);
	float w = sqrt(1 - chi0);
	chi0 = sqrt(chi0);
	float chi1 = Random(seed) * 6.283185307;
	float u = chi0 * cos(chi1);
	float v = chi0 * sin(chi1);
	
	// Compute normal and transform back to world space
	vec3 normal = u * m_i + v * m_j + w * m_k;
	return normalize(local * normal);
}
