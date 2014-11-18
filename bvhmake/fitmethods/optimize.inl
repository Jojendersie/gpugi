#include "../../utilities/random.hpp"

// create a population with stratified sampling.
template<unsigned N>
void populate(std::vector<Vec<N>>& _population, int _numNewIndividuals, const Vec<N>& _min, const Vec<N>& _max, uint64& _rndState)
{
	int cells = (int)pow(_numNewIndividuals, 1.0/N);
	// Compute numbers of individuals in cells (one per cell)
	int num = (int)pow(cells, N);
	if( cells > 0 )
	{
		// We need one nested loop per dimension. To iterate over all cells
		// simulate the N for-loop counters
		int x[N] = {0};
		for( int i = 0; i < num; ++i )
		{
			// Create a position inside cell x
			Vec<N> individal;
			for( unsigned d = 0; d < N; ++d )
				individal[d] = x[d] * (_max[d] - _min[d]) / cells
								+ Xorshift(_rndState, 0.0f, (_max[d] - _min[d])/cells);
			_population.push_back( individal );
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