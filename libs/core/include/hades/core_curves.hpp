#ifndef HADES_CORE_CURVES_HPP
#define HADES_CORE_CURVES_HPP

#include <tuple>

#include "hades/curve_extra.hpp"

namespace hades::data
{
	class data_manager;
}

namespace hades
{
	void register_core_curves(data::data_manager&);

	//NOTE: the get_* functions throw data::resource_null or data::resource_wrong_type
	const resources::curve* get_name_curve();
	const resources::curve* get_position_curve();
	const resources::curve* get_player_owner_curve();
	const resources::curve* get_size_curve();
	const resources::curve* get_collision_layer_curve();
	const resources::curve* get_tags_curve();

	constexpr auto bad_object_type = unique_zero;

	unique_id get_player_owner_id() noexcept;
	unique_id get_position_curve_id() noexcept;
	unique_id get_size_curve_id() noexcept;
	unique_id get_move_layers_id() noexcept;
	unique_id get_move_values_id() noexcept;
	unique_id get_collision_layer_curve_id() noexcept;
	unique_id get_tags_curve_id() noexcept;
}

#endif //!HADES_CORE_CURVES_HPP
