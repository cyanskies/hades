#ifndef HADES_DATAMANAGER_HPP
#define HADES_DATAMANAGER_HPP

#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/parser.hpp"
#include "Hades/resource_base.hpp"
#include "Hades/Types.hpp"
//Data manager is a resource management class.
//The class is not thread safe

namespace hades
{
	namespace resources
	{
		using parserFunc = std::function<void(unique_id mod, const YAML::Node& node, data::data_manager*)>;
		using parser_func = std::function<void(unique_id mod, const data::parser_node&, data::data_manager*)>;
	}

	namespace data
	{
		constexpr auto no_id_string = "ERROR_NO_UNIQUE_ID";

		class data_system final : public data_manager
		{
		public:
			data_system();

			//application registers the custom resource types
			//parser must convert yaml into a resource manifest object
			void register_resource_type(std::string_view name, resources::parserFunc parser);
			//TODO: move this to a virtual in the data class, so parsers can be registered without needing hades-core
			void register_resource_type(std::string_view name, resources::parser_func parser);

			//game is the name of a folder or archive containing a game.yaml file
			void load_game(std::string_view game);
			//mod is the name of a folder or archive containing a mod.yaml file
			void add_mod(std::string_view mod, bool autoLoad = false, std::string_view name = "mod.yaml");

			//returns true if the mod or game with that name has been parsed
			//note: this uses the mods self identified name, not the archive or folder name(as this can change in debug mode)
			bool loaded(std::string_view mod);

			//reread data from all mods, starting with game, and doing them all in order
			void reparse();

			//adds all objects into the load queue
			//note: this wont effect resources that are only parsed(strings and game parameters)
			void refresh() override;
			//adds a specific object into the load queue
			void refresh(unique_id) override;
			//adds a range to the load queue
			//template<Iter>
			//void refresh(Iter first, Iter last);

			//loads all queued resources
			void load();
			//loads a number of objects from the queue(no order specified)
			void load(types::uint8 count);
			//loads from the queue for a specified time
			//returns the number of objects loaded
			//>>types::uint32 load(Time ms);
			//loads a specific object(only if it is on the queue)
			void load(unique_id);
			//loads a range of objects(only if they are on the queue)
			//template<Iter>
			//void load(Iter first, Iter last);

			//convert string to uid
			types::string get_as_string(unique_id id) const override;
			unique_id get_uid(std::string_view name) const override;
			unique_id get_uid(std::string_view name) override;
			
		private:
			void parseYaml(unique_id, YAML::Node);
			void parseMod(std::string_view name, YAML::Node modRoot, std::function<bool(std::string_view)> dependency);

			//==parsing and loading data==
			std::unordered_map<types::string, resources::parserFunc> _resourceParsers;
			//==stored resource data==
			//list of used names
			std::unordered_set<types::string> _names;
			unique_id _game;
			std::vector<unique_id> _mods;
			//map of names to Uids
			std::unordered_map<types::string, unique_id> _ids;
			//list of unloaded resources
			std::vector<resources::resource_base*> _loadQueue;
		};

		//TODO: redo findorcreate and the yaml_* functions below, they shouldn't be in this header
		//they shouldnt accept pointers and should handle sequence specifiers correctly
		// probably in a resource_parser header

		//gets the requested resource, or creates it if not already
		// sets the resources most recent mod to the passed value
		// returns nullptr if unable to return a valid resource
		template<class T>
		T* FindOrCreate(unique_id target, unique_id mod, data_manager* data);
	}
}

//TODO: move all of these into a yaml_parser header and hades namespace.
template<class T, class Iter>
std::vector<const T*> convert_string_to_resource(Iter first, Iter last, hades::data::data_manager*);

//TODO: bool yaml_is_map
bool yaml_error(std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, std::string_view requested_type, hades::unique_id mod, bool test = false);

template<class T, typename ConversionFunc>
T yaml_get_scalar(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, hades::unique_id mod, ConversionFunc convert, T default_value);

template<class T>
T yaml_get_scalar(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, hades::unique_id mod, T default_value);

template<class T>
T yaml_get_scalar(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, T default_value)
{
	return yaml_get_scalar<T>(node, resource_type, resource_name, property_name, hades::unique_id::zero, default_value);
}


hades::unique_id yaml_get_uid(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, hades::unique_id mod = hades::unique_id::zero, hades::unique_id default_value = hades::unique_id::zero);

// value: [elm1, elm2] //same as '+' below
// value: [+, elm1, elm2] // to add to stored sequence
// value: [=, elm1, elm2] // to replace seqence
// value: [-, elm3, + elm1, elm2] // remove one element by name, then add two more
template<class T>
std::vector<T> yaml_get_sequence(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, hades::unique_id mod = hades::unique_id::zero);

template<class T, class ConversionFunc>
std::vector<T> yaml_get_sequence(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, ConversionFunc func, hades::unique_id mod = hades::unique_id::zero);

template<class T>
std::vector<T> yaml_get_sequence(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, const std::vector<T> &previous_sequence, hades::unique_id mod = hades::unique_id::zero);

template<class T, class ConversionFunc>
std::vector<T> yaml_get_sequence(const YAML::Node& node, std::string_view resource_type, std::string_view resource_name,
	std::string_view property_name, const std::vector<T> &previous_sequence, ConversionFunc func, hades::unique_id mod = hades::unique_id::zero);

#include "detail/data_system.inl"

#endif // hades_data_hpp
