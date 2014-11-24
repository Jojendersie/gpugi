#include "../../gpugi/utilities/random.hpp"

namespace PSO {

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
	float charge;			///< Weight for repulsion from other individuals
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
	_individual.charge = Xorshift(_rndState, 0.0f, 0.01f);
	// Keep momentum reasonable -> [0.3,0.7]
	_individual.damping = Xorshift(_rndState, 0.35f, 0.55f);
	// Create a velocity which is in 1%-2% of the domain size
	for( unsigned d = 0; d < N; ++d )
		_individual.velocity[d] = Xorshift(_rndState, _domainSize[d]*0.01f, _domainSize[d]*0.02f );
	// Reset memory
	_individual.optFitness = std::numeric_limits<float>::infinity();
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
			SwarmParticle<N> individual;
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
		SwarmParticle<N> individual;
		for( unsigned d = 0; d < N; ++d )
			individual.position[d] = Xorshift(_rndState, _min[d], _max[d]);
		randomizeParameters( individual, domainSize, _rndState );
		_population.push_back( individual );
	}
}

template<unsigned N>
void move(std::vector<SwarmParticle<N>>& _population, int _i, const Vec<N>& _optimum, float _optimalFitness, const Vec<N>& _min, const Vec<N>& _max, uint64& _rndState)
{
	float domainSize = len(_max-_min);
	// Rediversify: If the current one did not set the global optima last
	// turn but is very close to it jump.
	/*float distToOpt = len(m_population[_i].personalOpt - _optimum);
	if( distToOpt < domainSize*0.01f && &_population[_i] != m_ISetTheOptima )
		handleBoundary( m_population[_i], BoundaryHandling::RANDOMIZE );*/

	// Create 3 direction vectors
	Vec<N> towardsGlobalOpt = _optimum - _population[_i].position;
	Vec<N> towardsPersonalOpt = _population[_i].optPosition - _population[_i].position;
	Vec<N> randomPosition;
	for( unsigned d = 0; d < N; ++d )
		randomPosition[d] = Xorshift(_rndState, _min[d], _max[d]);
	// Normalize the 3 directions
	/*towardsGlobalOpt = towardsGlobalOpt / (towardsPersonalOpt.norm() + 1e-10);
	towardsPersonalOpt = towardsPersonalOpt / (towardsGlobalOpt.norm() + 1e-10);*/
	randomPosition = randomPosition * (domainSize / (len(randomPosition) + 1e-10f));
	_population[_i].velocity = _population[_i].velocity * _population[_i].damping
								+ towardsGlobalOpt * _population[_i].globalAttraction
								+ towardsPersonalOpt * _population[_i].localAttraction
								+ randomPosition * _population[_i].randomAttraction;

	// Compute repulsion from all others relative to the personal charge
	for( size_t j = 0; j < _population.size(); ++j ) if( _i != j )
	{
		Vec<N> jToi = _population[_i].position - _population[j].position;
		_population[_i].velocity += jToi * exp(-lensq(jToi) * 0.4f) * _population[_i].charge;
	}

	_population[_i].position = _population[_i].position + _population[_i].velocity;
	// Check new position if it is inside boundary and react otherwise
	for( unsigned d = 0; d < N; ++d )
		if( _population[_i].position[d] < _min[d] || _population[_i].position[d] > _max[d] )
		{
			_population[_i].position[d] = ε::clamp(_population[_i].position[d], _min[d], _max[d]);
			_population[_i].velocity[d] = -_population[_i].velocity[d];
		}
}


template<unsigned N>
void evaluateFitness(std::vector<SwarmParticle<N>>& _population, std::function<float(const Vec<N>&)> _function, Vec<N>& _globalOptimum, float& _globalFitness)
{
	for(size_t i = 0; i < _population.size(); ++i)
	{
		float value = _function(_population[i].position);
		if( value <= _population[i].optFitness )
		{
			_population[i].optFitness  = value;
			_population[i].optPosition = _population[i].position;
		}
		if( value < _globalFitness )
		{
			_globalFitness = value;
			_globalOptimum = _population[i].position;
		}
	}
}

} // namespace PSO

template<unsigned N>
Vec<N> optimize(const Vec<N>& _min, const Vec<N>& _max, std::function<float(const Vec<N>&)> _function, int _maxIterations)
{
	std::vector<PSO::SwarmParticle<N>> population;
	uint64 rndState;
	PSO::populate(population, 30, _min, _max, rndState);
	// no optimum for minimization problem
	Vec<N> globalOptimum;
	float globalFitness = std::numeric_limits<float>::infinity();

	// Iterate
	for(int turn = 0; turn < _maxIterations; ++turn)
	{
		// Evaluate initial (in first iteration) or new fitness and update
		// the global optimum.
		PSO::evaluateFitness( population, _function, globalOptimum, globalFitness );

		// Compute motion
		for(size_t i = 0; i < population.size(); ++i)
			PSO::move(population, (int)i, globalOptimum, globalFitness, _min, _max, rndState);
	}
	return globalOptimum;
}