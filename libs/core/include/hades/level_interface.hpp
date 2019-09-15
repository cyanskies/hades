#ifndef HADES_LEVEL_INTERFACE_HPP
#define HADES_LEVEL_INTERFACE_HPP

#include <exception>
#include <unordered_map>
#include <vector>

#include "hades/any_map.hpp"
#include "hades/curve_extra.hpp"
#include "hades/exceptions.hpp"
#include "hades/input.hpp"
#include "hades/level_curve_data.hpp"
#include "hades/game_system.hpp"
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

	//defines a subset of interface available to render instances
	class common_interface
	{
	public:
		virtual ~common_interface() = default;

		virtual curve_data &get_curves() noexcept = 0;
		virtual const curve_data &get_curves() const noexcept = 0;

		//gets an entity id from a unique name
		virtual entity_id get_entity_id(std::string_view, time_point t) const = 0;

		//map info
		//terrain()
		virtual const terrain_map& get_world_terrain() const noexcept = 0;
		virtual world_rect_t get_world_bounds() const noexcept = 0;
	};

	//TODO: implement common_interface for remote hosts,
	// just needs to accept and store data

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
		void add_input(input_system::action_set, time_point);

		curve_data& get_curves() noexcept override final;
		const curve_data& get_curves() const noexcept override final;

		const terrain_map& get_world_terrain() const noexcept override final;
		world_rect_t get_world_bounds() const noexcept override final;

	private:
		curve_data _curves;
		name_curve_t _entity_names{ curve_type::step };

		curve<input_system::action_set> _input{ curve_type::step };
		entity_id::value_type _next = static_cast<entity_id::value_type>(next(bad_entity));

		//level info
		terrain_map _terrain;
		world_vector_t _size;
	};

	//implements game_interface, this represents a game area,
	//with entities and systems
	//TODO: remove this from the implementation tree, and have it as a member of game_implementation
	template<typename SystemType>
	class common_implementation : public common_implementation_base
	{
	public:
		using system_type = SystemType;
		using system_resource = typename SystemType::system_t;

		explicit common_implementation(const level_save&);

		std::vector<SystemType> get_systems() const;
		std::vector<const system_resource*> get_new_systems() const;
		void clear_new_systems();

		system_data_t &get_system_data(unique_id);

		void attach_system(entity_id, unique_id, time_point t) override final;
		void detach_system(entity_id, unique_id, time_point t) override final;

	protected:
		std::vector<SystemType> _systems;
		std::vector<const system_resource*> _new_systems;
		std::unordered_map<unique_id, system_data_t> _system_data;
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

	private:
		system_behaviours<game_system> _systems;
	};

	//TODO: deprecate, render_instance replaces this entirely
	class render_implementation final : public common_implementation<render_system>
	{
	public:
		render_implementation();
	};

	template<typename Interface, typename SystemType, typename MakeGameStructFn>
	time_point update_level(time_point, time_point, time_duration,
		Interface&, system_behaviours<SystemType>&, MakeGameStructFn);
}

#include "hades/detail/level_interface.inl"

#endif //HADES_LEVEL_INTERFACE_HPP
