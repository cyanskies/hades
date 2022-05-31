#ifndef HADES_DATAMANAGER_HPP
#define HADES_DATAMANAGER_HPP

#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "hades/data.hpp"
#include "hades/parser.hpp"
#include "hades/resource_base.hpp"
#include "hades/types.hpp"

namespace hades::data
{
	class data_system final : public data_manager
	{
	public:
		data_system();

		//application registers the custom resource types
		//parser must convert yaml into a resource manifest object
		void register_resource_type(std::string_view name, resources::parser_func parser) override;

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
		void parseYaml(unique_id, const data::parser_node&);
		
		//==parsing and loading data==
		std::unordered_map<string, resources::parser_func> _resourceParsers;
		//==stored resource data==
		//list of used names
		std::unordered_set<string> _names;
		unique_id _game = unique_zero;
		std::vector<unique_id> _mods;
		unique_id _leaf_source = unique_zero; // temp loaded mod, this is the id of a mission file
		//map of names to Uids
		std::unordered_map<string, unique_id> _ids;
		//list of unloaded resources
		std::vector<resources::resource_base*> _loadQueue;
	};
}

#endif // hades_data_hpp
