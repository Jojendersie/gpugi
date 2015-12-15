
// Sample a random normal from an ellipsoid seen from a certain direction
vec3 sampleNormal(vec3 _incident, mat3x3 _sggx, inout uint _seed)
{
	// Compute an orthonormal basis around _incident
	mat3x3 local;
	local[2] = -_incident;
	CreateONB(_incident, local[0], local[1]);
	
	// Transform _sggx into the local system????
	float Sxx = dot(local[0], _sggx * local[0]);
	float Sxy = dot(local[0], _sggx * local[1]);
	float Sxz = dot(local[0], _sggx * local[2]);
	float Syy = dot(local[1], _sggx * local[1]);
	float Syz = dot(local[1], _sggx * local[2]);
	float Szz = dot(local[2], _sggx * local[2]);
	
	// Compute basis from sphere to NDF (Cholesky decomposition)
	vec3 m_i, m_j, m_k;
	float det = max(0.0, Sxx*Syy*Szz + Sxy*Syz*Sxz*2.0 - Sxz*Sxz*Syy - Sxy*Sxy*Szz - Syz*Syz*Sxx - DIVISOR_EPSILON);
	float tmp = Syy*Szz - Syz*Syz + DIVISOR_EPSILON;
	if(tmp != 0.0) m_i = vec3(sqrt(det / tmp), 0.0, 0.0);
	else m_i = vec3(0.0);
	tmp = sqrt(tmp);
	if(tmp != 0.0 && Szz != 0.0) m_j = vec3((Sxy*Szz-Sxz*Syz) / tmp, tmp, 0.0) / sqrt(Szz);
	else m_j = vec3(0.0);
	if(Szz != 0.0f) m_k = vec3(Sxz, Syz, Szz) / sqrt(Szz);
	else m_k = vec3(0.0);
	
	// Generate random point on visible hemisphere
	float chi0 = Random(_seed);
	float w = sqrt(1 - chi0);
	chi0 = sqrt(chi0);
	float chi1 = Random(_seed) * 6.283185307;
	float u = chi0 * cos(chi1);
	float v = chi0 * sin(chi1);
	
	// Compute normal and transform back to world space
	vec3 normal = u * m_i + v * m_j + w * m_k;
	return normalize(local * normal);
}
