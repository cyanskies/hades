#ifndef HADES_LEVEL_INTERFACE_HPP
#define HADES_LEVEL_INTERFACE_HPP

#include <exception>
#include <unordered_map>
#include <vector>

#include "hades/curve_extra.hpp"
#include "hades/exceptions.hpp"
#include "hades/game_state.hpp"
#include "hades/input.hpp"
#include "hades/level.hpp"
#include "hades/level_scripts.hpp"
#include "hades/game_system.hpp"
#include "hades/game_types.hpp"
#include "hades/terrain.hpp"
#include "hades/time.hpp"
#include "hades/types.hpp"

namespace hades
{
	constexpr auto world_entity_name = "world";

	class system_already_attached : public runtime_error
	{
		using runtime_error::runtime_error;
	};

	class system_not_attached : public runtime_error
	{
		using runtime_error::runtime_error;
	};

	class curve_not_registered : public runtime_error
	{
		using runtime_error::runtime_error;
	};

	struct level_save;

	class common_interface
	{
	public:
		virtual ~common_interface() noexcept = default;

		virtual const terrain_map& get_world_terrain() const noexcept = 0;
		virtual world_rect_t get_world_bounds() const noexcept = 0;
	};

	class game_interface : public common_interface
	{
	public:
		virtual object_ref create_object(const object_instance&) = 0;
		virtual object_ref get_object_ref(std::string_view) noexcept = 0;
	};

	//TODO: bring server functions out of the interface class
	//into the game_level func
	//
	// contains functions for the game to use
	class game_implementation : public game_interface
	{
	public:
		explicit game_implementation(const level_save&);

		object_ref create_object(const object_instance&) override;
		object_ref create_object(const resources::object&);

		void name_object(std::string_view, object_ref);

		system_behaviours<game_system>& get_systems() noexcept
		{ return _extras.systems; }

		object_ref get_object_ref(std::string_view) noexcept override;
		game_state& get_state() noexcept { return _state; }
		extra_state& get_extras() noexcept { return _extras; }
		const terrain_map& get_world_terrain() const noexcept override { return _terrain; }
		world_rect_t get_world_bounds() const noexcept override {
			return { {0.f, 0.f}, _size };
		}

		void update_input_queue(unique_id, std::vector<action> input, time_point);
		using input_queue_t = std::map<unique_id, std::vector<action>>;
		input_queue_t get_and_clear_input_queue() noexcept;

		resources::player_input::player_input_fn get_player_input_function() noexcept
		{ 
			if (_player_input)
				return _player_input->function;
			return nullptr;
		}

	private:
		game_state _state;
		extra_state _extras;

		//level info
		terrain_map _terrain;
		world_vector_t _size;

		//player input
		input_queue_t _input_queue;
		using action_history = game_property_curve<std::vector<action>>;
		//TODO: do we even use the history??? (debug data only i guess)
		std::map<unique_id, action_history> _input_history;
		const resources::player_input* _player_input{ nullptr };
	};

	// a level area
	class game_level
	{
	public:
	private:
	};

	struct player_data;

	template<typename Interface, typename SystemType, typename MakeGameStructFn>
	time_point update_level(time_point, time_point, time_duration,
		Interface&, system_behaviours<SystemType>&, const std::vector<player_data>*, MakeGameStructFn);
}

#include "hades/detail/level_interface.inl"

#endif //HADES_LEVEL_INTERFACE_HPP
