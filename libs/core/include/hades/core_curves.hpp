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

	//NOTE: the get_* functions throw curve_error
	// if the curve doesn't exist as a resource
	const resources::curve* get_name_curve();
	using vector_curve = std::tuple<const resources::curve*, const resources::curve*>;
	vector_curve get_position_curve();
	vector_curve get_size_curve();
	const resources::curve* get_collision_group_curve();
	const resources::curve* get_tags_curve();
	const resources::curve* get_object_type_curve();
}

#endif //!HADES_CORE_CURVES_HPP