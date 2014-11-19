#include "../../gpugi/utilities/random.hpp"

template<unsigned N>
struct SwarmParticle
{
	Vec<N> position;		///< This is the vector in the search space, i.a. the values to be optimized
	Vec<N> optPosition;		///< Personal optimum till now
	Vec<N> velocity;		///< Current direction and velocity
	float fitness;			///< Current fitness
	float optFitness;		///< Fitness at personal optimum
	float globalAttraction;	///< Weight for the global optimum attraction
	float localAttraction;	///< Weight for the personal optimum attraction
	float randomAttraction;	///< Weight for random targets
	float damping;			///< Momentum
};


// Randomizes the velocity and the parameters for attraction.
template<unsigned N>
void randomizeParameters(SwarmParticle<N>& _individual, const Vec<N>& _domainSize, uint64& _rndState)
{
	// Fill attraction values with random numbers
	_individual.globalAttraction = Xorshift(_rndState, 0.05f, 0.5f);
	_individual.localAttraction = Xorshift(_rndState, 0.0f, 0.25f);
	_individual.randomAttraction = Xorshift(_rndState, 0.0f, 0.001f);
	// Keep momentum reasonable -> [0.3,0.7]
	_individual.damping = Xorshift(_rndState, 0.35f, 0.55f);
	// Create a velocity which is in 1%-2% of the domain size
	for( unsigned d = 0; d < D; ++d )
		_individual.velocity[d] = Xorshift(_rndState, _domainSize[d]*0.01f, _domainSize[d]*0.02f );
}


// Create a population with stratified sampling.
template<unsigned N>
void populate(std::vector<SwarmParticle<N>>& _population, int _numNewIndividuals, const Vec<N>& _min, const Vec<N>& _max, uint64& _rndState)
{
	int cells = (int)pow(_numNewIndividuals, 1.0/N);
	// Compute numbers of individuals in cells (one per cell)
	int num = (int)pow(cells, N);
	Vec<N> domainSize = _max - _min;
	if( cells > 0 )
	{
		// We need one nested loop per dimension. To iterate over all cells
		// simulate the N for-loop counters
		int x[N] = {0};
		for( int i = 0; i < num; ++i )
		{
			// Create a position inside cell x
			SwarmParticle individual;
			for( unsigned d = 0; d < N; ++d )
				individual.position[d] = x[d] * domainSize[d] / cells
								+ Xorshift(_rndState, 0.0f, domainSize[d]/cells);
			randomizeParameters( individual, domainSize, _rndState );
			_population.push_back( individual );
			// Find the next cell - iterate in the smallest dimension first.
			// Then if the "loop" over the first #cell elements is finished
			// count up one in the next higher dimension and restart
			// iterating in the smaller dimensions.
			int d = 0;
			bool carry = true;
			while( carry && d<N )
			{
				// carry = "Loop" over dimension finished
				carry = ++x[d] == cells;
				if( carry ) x[d] = 0;
				++d;
			}
		}
	}

	// Distribute the remaining ones randomized
	for( int i = num; i < _numNewIndividuals; ++i )
	{
		// Create a position which is inside the domain
		Vec<N> individal;
		for( unsigned d = 0; d < N; ++d )
			individal[d] = Xorshift(_rndState, _min[d], _max[d]);
		_population.push_back( individal );
	}
}

template<unsigned N>
Vec<N> optimize(const Vec<N>& _min, const Vec<N>& _max, std::function<float(const Vec<N>&)> _function)
{
	std::vector<Vec<N>>& population;
	uint64 rndState;
	populate(population, 30, _min, _max, rndState);
	return _min;
}