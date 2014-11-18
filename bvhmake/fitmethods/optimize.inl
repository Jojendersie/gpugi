// create a population with stratified sampling.
void populate();

template<unsigned N>
Vec<N> optimize(Vec<N> _min, Vec<N> _max, std::function<float(const Vec<N>&)> _function)
{
	return _min;
}