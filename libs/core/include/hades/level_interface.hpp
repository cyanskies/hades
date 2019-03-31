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
#include "hades/shared_guard.hpp"
#include "hades/shared_map.hpp"
//#include "Hades/simple_resources.hpp"
#include "hades/game_system.hpp"
#include "hades/timers.hpp"
#include "hades/types.hpp"

namespace hades
{
	struct curve_data
	{
		template<class T>
		using curve_map = shared_map< std::pair<entity_id, variable_id>, curve<T> >;	

		curve_map<resources::curve_types::int_t> int_curves;
		curve_map<resources::curve_types::float_t> float_curves;
		//no linear curves here
		curve_map<resources::curve_types::bool_t> bool_curves;
		curve_map<resources::curve_types::string> string_curves;
		curve_map<resources::curve_types::object_ref> object_ref_curves;
		curve_map<resources::curve_types::unique> unique_curves;

		//no linear curves here either
		curve_map<resources::curve_types::vector_int> int_vector_curves;
		curve_map<resources::curve_types::vector_float> float_vector_curves;
		curve_map<resources::curve_types::vector_object_ref> object_ref_vector_curves;
		curve_map<resources::curve_types::vector_unique> unique_vector_curves;
	};

	template<typename T>
	curve_data::curve_map<T> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::int_t> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::float_t> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::bool_t> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::string> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::object_ref> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::unique> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_int> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_float> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_object_ref> &get_curve_list(curve_data &data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_unique> &get_curve_list(curve_data &data);

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
	using name_curve_t = curve<std::map<types::string, entity_id>>;

	struct level_save;

	//this is the interface that is available to jobs and systems
	//it supports multi threading the whole way though
	class game_interface
	{
	public:
		virtual ~game_interface() {}

		game_interface(const level_save&);

		//creates an entity with no curves or systems attached to it
		entity_id create_entity();
		entity_id get_entity_id(const types::string &name, time_point t) const;

		curve_data &get_curves();
		const curve_data &get_curves() const;

		//const property_map &getProperties() const;

		//attach/detach entities from systems
		void attach_system(entity_id, unique_id, time_point t);
		void detach_system(entity_id, unique_id, time_point t);

	protected:
		//TODO: do these need to remain?
		game_system& FindSystem(unique_id);
		game_system& install_system(unique_id sys);

		shared_guard<name_curve_t> _entity_names = name_curve_t{ curve_type::step };
		mutable std::mutex _system_list_mut;
		std::vector<game_system> _systems;

		std::atomic<entity_id::value_type> _next = static_cast<entity_id::value_type>(bad_entity) + 1;
	private:
		//CURVE VARIABLES
		curve_data _curves;
		//shared properties
		//property_map _properties;
	};

	void InstallSystem(resources::system*, std::shared_mutex&, std::vector<game_system>&);
}

#include "hades/detail/level_interface.inl"

#endif //HADES_LEVEL_INTERFACE_HPP
