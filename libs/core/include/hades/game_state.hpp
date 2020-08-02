#ifndef HADES_GAME_STATE_HPP
#define HADES_GAME_STATE_HPP

#include "plf_colony.h"

#include "hades/any_map.hpp"
#include "hades/curve_types.hpp"
#include "hades/game_system.hpp"
#include "hades/uniqueid.hpp"

namespace hades
{
	//this isn't needed for EntityId's and Entity names are strings, and rarely used, where
	//curves need to be identified often by a consistant lookup name
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using variable_id = unique_id;
	constexpr auto bad_variable = variable_id{ variable_id::zero_id };

	template<typename DataType>
	struct state_field
	{
		entity_id object = bad_entity;
		variable_id id = bad_variable;
		DataType data;
	};

	namespace detail
	{
		template<typename T>
		struct var_entry
		{
			variable_id id = bad_variable;
			state_field<T>* var = nullptr;
		};

		template<typename T>
		constexpr bool operator==(const var_entry<T>& l, const var_entry<T>& r) noexcept
		{ return l.id == r.ld; }

		template<typename T>
		struct entity_var_type;

		template<typename... Ts>
		struct entity_var_type<std::tuple<Ts...>>
		{
			using type = std::tuple<std::vector<var_entry<Ts>>...>;
		};

		template<typename T>
		using entity_var_type_t = typename entity_var_type<T>::type;
	}

	//authoratative game object
	struct game_obj
	{
		entity_id id = bad_entity;
		const resources::object* object_type = nullptr; //tells what variables a entity has, and which systems control it.

		template<typename T>
		using var_entry = detail::var_entry<T>;
		template<typename T>
		using var_list = std::vector<var_entry<T>>;

		using entity_variable_lists_t = detail::entity_var_type_t<curve_types::type_pack>;
		entity_variable_lists_t object_variables;
	};

	inline constexpr bool operator==(const game_obj& lhs, const game_obj &rhs) noexcept
	{
		return lhs.id == rhs.id;
	}

	using object_name_map = std::unordered_map<string, object_ref>;

	namespace detail
	{
		template<typename DataType>
		using data_colony = plf::colony<state_field<DataType>>;

		template<typename T> struct state_data_type;
		template<typename... Ts>
		struct state_data_type<std::tuple<Ts...>>
		{
			using type = std::tuple<data_colony<Ts>...>;
		};

		template<typename T>
		using state_data_type_t = typename state_data_type<T>::type;
	}

	//the whole game state, this is everything that gets saved
	// the only data not held here, is from the level file(terrain data)
	// and calculated state in extra_state
	struct game_state
	{
		template<typename T>
		using data_colony = detail::data_colony<T>;
		using state_data_type = detail::state_data_type_t<curve_types::type_pack>;
		state_data_type state_data;
		entity_id next_id = next(bad_entity);
		object_name_map names;
	};

	// non-saved state, this is generated during runtime from the actual game state.
	// this doesn't need to be sent to clients, they load or generate their own extras
	struct extra_state
	{
		plf::colony<game_obj> objects;
		//system behaviours, including system local data
		system_behaviours<game_system> systems;
		//level local data, available in all systems
		any_map<unique_id> level_locals;
	};

	//functions for modifying game state
	namespace state_api
	{
		object_ref make_object(const object_instance&, game_state&, extra_state&);
		//returns true if the object was named, false if the name is already taken
		bool name_object(string, object_ref, game_state&);
		object_ref get_object_ref(std::string_view, game_state&, extra_state&) noexcept;
		game_obj* get_object(object_ref, extra_state&) noexcept;
		template<typename T>
		T& get_object_property_ref(game_obj&, variable_id) noexcept;
	}
}

#endif //!HADES_GAME_STATE_HPP
