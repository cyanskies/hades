#ifndef HADES_LEVEL_INTERFACE_HPP
#define HADES_LEVEL_INTERFACE_HPP

#include <atomic>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "hades/any_map.hpp"
#include "hades/curve_extra.hpp"
#include "hades/exceptions.hpp"
#include "hades/input.hpp"
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

		virtual curve_data &get_curves() noexcept = 0;
		virtual const curve_data &get_curves() const noexcept = 0;
	};

	//this is the interface that is available to server jobs and systems
	//it supports multi threading the whole way though
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
		virtual entity_id get_entity_id(std::string_view, time_point t) const = 0;

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
		mutable std::mutex _system_list_mut;
		std::vector<SystemType> _systems;
		std::vector<const system_resource*> _new_systems;
		std::unordered_map<unique_id, system_data_t> _system_data;
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

	namespace update_level_tags
	{
		struct update_level_auto_tag {};
		constexpr update_level_auto_tag auto_tag;
		struct update_level_async_tag {};
		constexpr update_level_async_tag async_tag;
		struct update_level_noasync_tag {};
		constexpr update_level_noasync_tag noasync_tag;
	}

	template<typename ImplementationType, typename MakeGameStructFn, typename ModeTag = update_level_tags::update_level_auto_tag>
	time_point update_level(job_system&, time_point, time_point, time_duration,
		ImplementationType&, MakeGameStructFn, ModeTag = update_level_tags::auto_tag);
}

#include "hades/detail/level_interface.inl"

#endif //HADES_LEVEL_INTERFACE_HPP
