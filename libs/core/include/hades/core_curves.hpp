#ifndef HADES_CORE_CURVES_HPP
#define HADES_CORE_CURVE_HPP

#include <tuple>

#include "hades/curve_extra.hpp"

namespace hades::data
{
	class data_manager;
}

namespace hades
{
	//NOTE: curve resources should be registered before calling this
	void register_core_curves(data::data_manager&);

	const resources::curve* get_name_curve();
	using vector_curve = std::tuple<const resources::curve*, const resources::curve*>;
	vector_curve get_position_curve();
	vector_curve get_size_curve();
}

#endif //!HADES_CORE_CURVES_HPP