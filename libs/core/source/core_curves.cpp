#include "hades/core_curves.hpp"
#include "hades/data.hpp"

namespace hades
{
	static auto name_id = unique_zero;
	static auto pos_id = unique_zero;
	static auto siz_id = unique_zero;
	static auto player_owner_id = unique_zero;
	static auto collision_groups_id = unique_zero;
	static auto tags_id = unique_zero;

	static void setup_position_curve(data::data_manager& d, unique_id id)
	{
		resources::make_curve(d, id,
			resources::curve_variable_type::vec2_float,
			keyframe_style::linear,
			resources::curve_types::vec2_float{ 0.f, 0.f },
			true, true);
		return;
	}

	void register_core_curves(data::data_manager &d)
	{
		register_curve_resource(d);

		using namespace std::string_view_literals;
		using resources::curve;

		// the objects name, usually the name of the object type
		name_id = d.get_uid("name"sv);
		resources::make_curve(d, name_id, resources::curve_variable_type::string,
			keyframe_style::const_t, string{}, true, true);

		// the position curves store the objects 2d position
		pos_id = d.get_uid("position"sv);
		setup_position_curve(d, pos_id);
		// size curves store the objects 2d size
		siz_id = d.get_uid("size"sv);
		resources::make_curve(d, siz_id,
			resources::curve_variable_type::vec2_float,
			keyframe_style::const_t,
			resources::curve_types::vec2_float{ 0.f, 0.f },
			true, false);
		
		// collision groups are used to check what terrain an object can move on
		// also used by the editor to know where an object can be placed
		//TODO: should a sperate collision-terrain list be used for terrain collision?
		collision_groups_id = d.get_uid("collision-groups"sv);
		resources::make_curve(d, collision_groups_id,
			resources::curve_variable_type::collection_unique,
			keyframe_style::const_t,
			resources::curve_types::collection_unique{},
			false, //sync to client
			false); //save to file

		// tags for this object
		tags_id = d.get_uid("tags"sv);
		resources::make_curve(d, tags_id,
			resources::curve_variable_type::collection_unique,
			keyframe_style::const_t,
			resources::curve_types::collection_unique{},
			true, // sync to client
			false); // save to file

		player_owner_id = d.get_uid("player-owner"sv);
		resources::make_curve(d, player_owner_id,
			resources::curve_variable_type::unique,
			keyframe_style::step,
			resources::curve_types::bad_unique,
			true,
			true);

		return;
	}

	static const resources::curve *get_curve(unique_id i)
	{
		// if !i then register core curves was never called
		assert(i);
		return data::get<resources::curve>(i);
	}

	const resources::curve* get_name_curve()
	{
		return get_curve(name_id);
	}

	const resources::curve* get_position_curve()
	{
		return get_curve(pos_id);
	}

	const resources::curve* get_player_owner_curve()
	{
		return get_curve(player_owner_id);
	}
	
	const resources::curve* get_size_curve()
	{
		return get_curve(siz_id);
	}

	const resources::curve * get_collision_group_curve()
	{
		return get_curve(collision_groups_id);
	}

	const resources::curve *get_tags_curve()
	{
		return get_curve(tags_id);
	}

	unique_id get_position_curve_id() noexcept
	{
		return pos_id;
	}
	
	unique_id get_player_owner_id() noexcept
	{
		return player_owner_id;
	}

	unique_id get_size_curve_id() noexcept
	{
		return siz_id;
	}

	unique_id get_collision_group_curve_id() noexcept
	{
		return collision_groups_id;
	}

	unique_id get_tags_curve_id() noexcept
	{
		return tags_id;
	}
}
