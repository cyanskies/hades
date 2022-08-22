#include "hades/random.hpp"

namespace hades::detail
{
	struct random_data
	{
		std::random_device rd{};
		// TODO: PCG random engine
		std::default_random_engine random_generator{ rd() };
	};

	thread_local static random_data random_runtime_data = {};
	std::default_random_engine& get_engine() noexcept
	{
		return random_runtime_data.random_generator;
	}
}
