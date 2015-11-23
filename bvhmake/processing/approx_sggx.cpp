#include "approx_sggx.hpp"

using namespace ε;

/// \brief Linera interpolable values of the symmetrix 3x3 matrix.
/// \details This uncompressed form (contrary to FileDecl::SGGX) serves as
///		intermediate result.
struct SGGX
{
	float xx, xy, xz, yy, yz, zz;
};

inline int rev(const int i, const int p) {if (i==0) return i; else return p-i;}
float halton(int _base, int _i)
{
	float h = 0.0;
	const float f = 1.0f / _base;
	float fct = f;
	while(_i > 0) {
		h += rev(_i % _base, _base) * fct;
		_i /= _base;
		fct *= f;
	}
	return h;
}

const int NORMAL_SAMPLES = 1000;

SGGX ComputeLeafSGGXBase(const BVHBuilder* _bvhBuilder, const FileDecl::Leaf& _leaf)
{
	// Load triangle geometry
	Triangle pos[FileDecl::Leaf::NUM_PRIMITIVES];
	Vec3 nrm[FileDecl::Leaf::NUM_PRIMITIVES * 3];
	int n = 0; // Count real number of triangles
	for( ; n < FileDecl::Leaf::NUM_PRIMITIVES; ++n)
	{
		if(_leaf.triangles[n] == FileDecl::INVALID_TRIANGLE) break;
		pos[n] = _bvhBuilder->GetTriangle( _leaf.triangles[n] );
		nrm[n*3    ] = _bvhBuilder->GetNormal( _leaf.triangles[n].vertices[0] );
		nrm[n*3 + 1] = _bvhBuilder->GetNormal( _leaf.triangles[n].vertices[1] );
		nrm[n*3 + 2] = _bvhBuilder->GetNormal( _leaf.triangles[n].vertices[2] );
	}

	// Sample random normals from all triangles and compute spherical distribution variances.
	Mat3x3 E(0.0f); // Expectations, later covariance matrix
	float area = surface(pos[0]);
	for(int i = 1; i < n; ++i) area += surface(pos[i]);
	for(int i = 0; i < n; ++i)
	{
		// Get percentage of the total number of samples
		int nsamples = int(NORMAL_SAMPLES * surface(pos[i]) / area);
		for(int j = 0; j < nsamples; ++j)
		{
			// Create uniform barycentric sample
			float α = halton(2, j);
			float β = halton(3, j);
			if(α + β > 1.0f) { α = 1.0f - α; β = 1.0f - β; }
			float γ = 1.0f - α - β;
			Vec3 normal = normalize(nrm[i*3] * α + nrm[i*3+1] * β + nrm[i*3+1] * γ);
			E.m00 += normal.x * normal.x;
			E.m01 += normal.x * normal.y;
			E.m02 += normal.x * normal.z;
			E.m11 += normal.y * normal.y;
			E.m12 += normal.y * normal.z;
			E.m22 += normal.z * normal.z;
		}
	}
	// Copy symmetric part of the matrix. Do not need to normalize by sample
	// count because the scale (eigenvalues) are not of interest.
	E.m10 = E.m01; E.m20 = E.m02; E.m21 = E.m12;
	// Get eigenvectors they are the same as for the SGGX base
	Mat3x3 Q; Vec3 λ;
	decomposeQl(E, Q, λ);

	// Compute projected areas in the directions of eigenvectors using the same
	// distribution as before.
	λ = Vec3(0.0f);
	int nTotalSamples = 0;
	for(int i = 0; i < n; ++i)
	{
		// Get percentage of the total number of samples
		int nsamples = int(NORMAL_SAMPLES * surface(pos[i]) / area);
		nTotalSamples += nsamples;
		for(int j = 0; j < nsamples; ++j)
		{
			// Create uniform barycentric sample
			float α = halton(2, j);
			float β = halton(3, j);
			if(α + β > 1.0f) { α = 1.0f - α; β = 1.0f - β; }
			float γ = 1.0f - α - β;
			Vec3 normal = normalize(nrm[i*3] * α + nrm[i*3+1] * β + nrm[i*3+1] * γ);
			λ[0] += dot(transpose(Q(0)), normal);
			λ[1] += dot(transpose(Q(1)), normal);
			λ[2] += dot(transpose(Q(2)), normal);
		}
	}
	λ /= nTotalSamples;// TODO: /4π ?
	E = transpose(Q) * diag(λ) * Q;
	SGGX s;
	s.xx = E.m00; s.xy = E.m01; s.xz = E.m02;
				  s.yy = E.m11; s.yz = E.m12;
								s.zz = E.m22;
	Assert(s.xx >= 0.0f && s.xx <= 1.0f, "Value Sxx outside expected range!");
	Assert(s.yy >= 0.0f && s.yy <= 1.0f, "Value Syy outside expected range!");
	Assert(s.zz >= 0.0f && s.zz <= 1.0f, "Value Szz outside expected range!");
	Assert(s.xy >= -1.0f && s.xy <= 1.0f, "Value Sxy outside expected range!");
	Assert(s.xz >= -1.0f && s.xz <= 1.0f, "Value Sxz outside expected range!");
	Assert(s.yz >= -1.0f && s.yz <= 1.0f, "Value Syz outside expected range!");
	return s;
}

SGGX RecursiveComputeSGGXBases(const BVHBuilder* _bvhBuilder, uint32 _index, std::vector<FileDecl::SGGX>& _output)
{
	SGGX s;
	// End of recursion (inner node which points to one leaf)
	if(_bvhBuilder->GetNode(_index).left & 0x80000000)
	{
		s = ComputeLeafSGGXBase(_bvhBuilder, _bvhBuilder->GetLeaf(_bvhBuilder->GetNode(_index).left & 0x7fffffff));
	} else {
		s =  RecursiveComputeSGGXBases(_bvhBuilder, _bvhBuilder->GetNode(_index).left, _output);
		SGGX s2 = RecursiveComputeSGGXBases(_bvhBuilder, _bvhBuilder->GetNode(_index).right, _output);
		// Weight depending on subtree bounding volume sizes
		auto fit = _bvhBuilder->GetFitMethod();
		float lw = fit->Surface( _bvhBuilder->GetNode(_index).left );
		float rw = fit->Surface( _bvhBuilder->GetNode(_index).right );
		float wsum = lw + rw;
		lw /= wsum; rw /= wsum;
		s.xx = s.xx * lw + s2.xx * rw;
		s.xy = s.xy * lw + s2.xy * rw;
		s.xz = s.xz * lw + s2.xz * rw;
		s.yy = s.yy * lw + s2.yy * rw;
		s.yz = s.yz * lw + s2.yz * rw;
		s.zz = s.zz * lw + s2.zz * rw;
	}
	// Store compressed form
	_output[_index].σ = Vec<int16, 3>(sqrt(Vec3(s.xx, s.yy, s.zz)) * 65535.0f - 32768.0f);
	_output[_index].r = Vec<int16, 3>(Vec3(s.xy, s.xz, s.yz) / sqrt(Vec3(s.xx*s.yy, s.xx*s.zz, s.yy*s.zz)) * 32767.0f);
	return s;
}

void ComputeSGGXBases(const BVHBuilder* _bvhBuilder,
	std::vector<FileDecl::SGGX>& _output)
{
	// The number of output nodes is known in advance.
	// After resize writing is possible with random access.
	_output.resize(_bvhBuilder->GetNumNodes());
}