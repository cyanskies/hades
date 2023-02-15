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

	/*class system_already_attached : public runtime_error
	{
		using runtime_error::runtime_error;
	};

	class system_not_attached : public runtime_error
	{
		using runtime_error::runtime_error;
	};*/

	class curve_not_registered : public runtime_error
	{
		using runtime_error::runtime_error;
	};

	struct level_save;

	//used by render and game
	class common_interface
	{
	public:
		virtual ~common_interface() noexcept = default;

		virtual const terrain_map& get_world_terrain() const noexcept = 0;
		virtual world_rect_t get_world_bounds() const noexcept = 0;

		virtual const game_state& get_state() const noexcept = 0;
		virtual game_state& get_state() noexcept = 0;
		virtual std::vector<game_obj> get_new_objects() noexcept = 0;
		virtual std::vector<entity_id> get_removed_objects() noexcept = 0;
	};

	//use by game
	class game_interface : public common_interface
	{
	public:
		virtual object_ref create_object(const object_instance&, time_point) = 0;
		virtual object_ref clone_object(object_ref, time_point) = 0;
		virtual void destroy_object(object_ref, time_point) = 0;

		virtual object_ref get_object_ref(std::string_view, time_point) noexcept = 0;
		virtual extra_state<game_system>& get_extras() noexcept = 0;
	};

	//game server imp
	// TODO: final
	class game_implementation : public game_interface
	{
	public:
		game_implementation(const level_save&, game_interface* mission,
			const std::vector<player_data>* players);

		game_implementation(game_implementation&&) noexcept = default;
		game_implementation& operator=(game_implementation&&) noexcept = default;

		~game_implementation();

		object_ref create_object(const object_instance&, time_point) override;
		object_ref clone_object(object_ref, time_point) override;
		void destroy_object(object_ref, time_point) override;

		std::vector<game_obj*> get_destroyed_objects() noexcept;
		std::vector<game_obj> get_new_objects() noexcept override;
		std::vector<entity_id> get_removed_objects() noexcept override;

		void name_object(std::string_view, object_ref, time_point);

		system_behaviours<game_system>& get_systems() noexcept
		{ return _extras.systems; }

		object_ref get_object_ref(std::string_view, time_point) noexcept override;
		const game_state& get_state() const noexcept override { return _state; }
		game_state& get_state() noexcept override { return _state; }
		extra_state<game_system>& get_extras() noexcept override { return _extras; }
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
		extra_state<game_system> _extras;
		// NOTE: for local clients to access
		// should be removed for dedicated server
		// should be stored at the server_level?
		std::vector<game_obj> _new_objects;
		std::vector<game_obj*> _destroy_objects;
		std::vector<entity_id> _removed_objects;

		//level info
		terrain_map _terrain;
		world_vector_t _size;

		//player input
		input_queue_t _input_queue;
		//using action_history = step_curve<std::vector<action>>;
		//TODO: do we even use the history??? (debug data only i guess)
		//std::map<unique_id, action_history> _input_history;
		const resources::player_input* _player_input{ nullptr };
	};

	struct player_data;

	// always call this at least once after loading a save file,
	// before update_level
	template<typename Interface, typename JobDataType, typename MakeGameStructFn>
	void create_systems(JobDataType, time_duration,
		Interface&, Interface* mission, const std::vector<player_data>*, MakeGameStructFn&&);

	// advances the game time by time_duration
	template<typename Interface, typename JobDataType, typename MakeGameStructFn>
	time_point update_level(JobDataType, time_duration,
		Interface&, Interface* mission, const std::vector<player_data>*, MakeGameStructFn&&);
}

#include "hades/detail/level_interface.inl"

#endif //HADES_LEVEL_INTERFACE_HPP
