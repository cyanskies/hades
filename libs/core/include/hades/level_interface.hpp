#ifndef HADES_LEVEL_INTERFACE_HPP
#define HADES_LEVEL_INTERFACE_HPP

#include <atomic>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "hades/any_map.hpp"
#include "hades/curve_extra.hpp"
#include "hades/exceptions.hpp"
#include "hades/level_curve_data.hpp"
#include "hades/shared_guard.hpp"
#include "hades/shared_map.hpp"
#include "hades/game_system.hpp"
#include "hades/timers.hpp"
#include "hades/types.hpp"

namespace hades
{
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
	//the members of curve_data are all thread safe, so this interface is too
	class common_interface
	{
	public:
		virtual ~common_interface() = default;

		virtual curve_data &get_curves() = 0;
		virtual const curve_data &get_curves() const = 0;
	};

	//this is the interface that is available to server jobs and systems
	//it supports multi threading the whole way though
	class game_interface : public common_interface
	{
	public:
		//creates an entity with no curves or systems attached to it
		virtual entity_id create_entity() = 0;
		//virtual entity_id create_entity(const resources::object*) = 0;
		virtual entity_id get_entity_id(std::string_view, time_point t) const = 0;

		//adds all the curves and effects of object* to entity_id
		//doesnt replace curves that are already present
		//virtual void update_entity(entity_id, const resources::object*) = 0;

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
		entity_id get_entity_id(std::string_view, time_point t) const override final;

		void name_entity(entity_id, std::string_view, time_point);
		void add_input(input_system::action_set, time_point);

		curve_data& get_curves() override final;
		const curve_data& get_curves() const override final;

	private:
		curve_data _curves;
		shared_guard<name_curve_t> _entity_names = name_curve_t{ curve_type::step };

		curve<input_system::action_set> _input{ curve_type::step };
		std::atomic<entity_id::value_type> _next = static_cast<entity_id::value_type>(next(bad_entity));
	};

	//implements game_interface, this represents a game area,
	//with entities and systems
	template<typename SystemType>
	class common_implementation : public common_implementation_base
	{
	public:
		explicit common_implementation(const level_save&);

		std::vector<SystemType> get_systems() const;

		void attach_system(entity_id, unique_id, time_point t) override final;
		void detach_system(entity_id, unique_id, time_point t) override final;

	protected:
		mutable std::mutex _system_list_mut;
		std::vector<SystemType> _systems;
	};

	class game_implementation final : public common_implementation<game_system>
	{
	public:
		explicit game_implementation(const level_save&);
	};

	class render_implementation final : public common_implementation<render_system>
	{
	public:
		render_implementation();
	};
}

#include "hades/detail/level_interface.inl"

#endif //HADES_LEVEL_INTERFACE_HPP
