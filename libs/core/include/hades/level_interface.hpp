#ifndef HADES_LEVEL_INTERFACE_HPP
#define HADES_LEVEL_INTERFACE_HPP

#include <exception>
#include <unordered_map>
#include <vector>

#include "hades/any_map.hpp"
#include "hades/curve_extra.hpp"
#include "hades/exceptions.hpp"
#include "hades/input.hpp"
#include "hades/level.hpp"
#include "hades/level_curve_data.hpp"
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

	using property_map = any_map<unique_id>;
	
	//TODO: these should be either an interface or final

	class client_interface
	{
	public:
		virtual ~client_interface() = default;

		// constant curve access
		virtual const curve_data& get_curves() const noexcept = 0;

		//gets an entity id from a unique name
		virtual entity_id get_entity_id(std::string_view, time_point t) const = 0;

		//map info
		virtual const terrain_map& get_world_terrain() const noexcept = 0;
		virtual world_rect_t get_world_bounds() const noexcept = 0;

		// non-persistant level local storage
		// data stored here is not recorded in saves
		// and should not be assumed to be available
		virtual std::any& get_level_local_ref(unique_id) = 0;
		virtual void set_level_local_value(unique_id, std::any) = 0;
	private:
		//TODO, impl level_local_ref/value here
	};

	//defines a subset of interface available to render instances
	class common_interface : public client_interface
	{
	public:
		// adds mutable curve access, so the interface can be used by
		// server side functions as well
		virtual curve_data &get_curves() noexcept = 0;
	};

	//TODO: implement common_interface for remote hosts,
	// just needs to accept and store data
	// this is the client side curve storage
	// merge the server game implementation into a single class

	class game_interface : public common_interface
	{
	public:
		//creates an entity with no curves or systems attached to it
		virtual entity_id create_entity() = 0;
		//writes curves and other settings from object_instance into entity id
		//if any of the curves already exist then they will be skipped,
		//entity name will also not be applied
		virtual entity_id create_entity(const object_instance&, time_point) = 0;
		//virtual entity_id create_entity(const resources::object*) = 0;
		
		//TODO: deprecate
		//attach/detach entities from systems
		virtual void attach_system(entity_id, unique_id, time_point t) = 0;
		virtual void detach_system(entity_id, unique_id, time_point t) = 0;
	};

	struct level_save;

	class common_implementation_base : public game_interface
	{
	public:
		explicit common_implementation_base(const level_save&);

		entity_id create_entity() override final;
		entity_id create_entity(const object_instance&, time_point) override final;
		entity_id get_entity_id(std::string_view, time_point t) const override final;

		void name_entity(entity_id, std::string_view, time_point);

		curve_data& get_curves() noexcept override final;
		const curve_data& get_curves() const noexcept override final;

		const terrain_map& get_world_terrain() const noexcept override final;
		world_rect_t get_world_bounds() const noexcept override final;

		std::any& get_level_local_ref(unique_id) override final;
		void set_level_local_value(unique_id u, std::any a) override final
		{
			_level_local.insert_or_assign(u, std::move(a));
			return;
		}

	private:
		curve_data _curves;
		name_curve_t _entity_names{ curve_type::step };

		curve<input_system::action_set> _input{ curve_type::step };
		entity_id::value_type _next = to_value(next(bad_entity));

		//level info
		terrain_map _terrain;
		world_vector_t _size;

		std::unordered_map<unique_id, std::any> _level_local;
	};

	class game_implementation final : public common_implementation_base
	{
	public:
		explicit game_implementation(const level_save&);

		void attach_system(entity_id e, unique_id i, time_point t) override
		{ _systems.attach_system(e, i, t); }
		void detach_system(entity_id e, unique_id i, time_point t) override
		{ _systems.detach_system(e, i, t); }

		system_behaviours<game_system>& get_systems() noexcept
		{ return _systems; }

		void update_input_queue(std::vector<server_action> input, time_point);
		std::vector<server_action> get_and_clear_input_queue() noexcept;

		resources::player_input::player_input_fn get_player_input_function() noexcept
		{ 
			if (_player_input)
				return _player_input->function;
			return nullptr;
		}

	private:
		system_behaviours<game_system> _systems;
		std::vector<server_action> _input_queue;
		using action_history = std::pair<std::vector<server_action>, time_point>;
		std::vector<action_history> _input_history;
		const resources::player_input* _player_input{ nullptr };
		//ai_input
	};

	template<typename Interface, typename SystemType, typename MakeGameStructFn>
	time_point update_level(time_point, time_point, time_duration,
		Interface&, system_behaviours<SystemType>&, MakeGameStructFn);
}

#include "hades/detail/level_interface.inl"

#endif //HADES_LEVEL_INTERFACE_HPP
