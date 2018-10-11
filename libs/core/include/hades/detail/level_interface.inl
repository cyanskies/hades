#include "hades/level_interface.hpp"

namespace hades
{
	template<typename T>
	curve_data::curve_map<T> &get_curve_list(curve_data &data)
	{
		assert(false);
		throw std::logic_error{"invalid type"};
	}
}