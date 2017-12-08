#ifndef HADES_DATAMANAGER_HPP
#define HADES_DATAMANAGER_HPP

#include <unordered_map>
#include <unordered_set>

#include "yaml-cpp/yaml.h"

#include "Hades/type_erasure.hpp"
#include "Hades/resource_base.hpp"

//Data manager is a resource management class.
//The class is not thread safe

namespace hades
{
	namespace resources
	{
		struct mod_t {};

		struct mod : public resource_type<mod_t>
		{
			// source == the name of the archive containing the mode
			// dependencies: a list of mods this mod depends on
			std::vector<data::UniqueId> dependencies,
				//names: unique id's provided by this mod
				names;

			std::string name;
			//value is unused
			//mod_t value
		};
	}

	namespace data
	{
		const types::string no_id_string = "ERROR_NO_UNIQUE_ID";

		class data_manager
		{
		public:
			data_manager();

			virtual ~data_manager();
			//application registers the custom resource types
			//parser must convert yaml into a resource manifest object
			void register_resource_type(std::string name, resources::parserFunc parser);
	
			//game is the name of a folder or archive containing a game.yaml file
			void load_game(std::string game);
			//mod is the name of a folder or archive containing a mod.yaml file
			void add_mod(std::string mod, bool autoLoad = false, std::string name = "mod.yaml");

			//returns true if the mod or game with that name has been parsed
			//note: this uses the mods self identified name, not the archive or folder name(as this can change in debug mode)
			bool loaded(std::string mod);

			//reread data from all mods, starting with game, and doing them all in order
			void reparse();

			//adds all objects into the load queue
			//note: this wont effect resources that are only parsed(strings and game parameters)
			void refresh();
			//adds a specific object into the load queue
			void refresh(UniqueId);
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
			void load(UniqueId);
			//loads a range of objects(only if they are on the queue)
			//template<Iter>
			//void load(Iter first, Iter last);

			template<class T>
			void set(UniqueId, std::unique_ptr<T>);

			bool exists(UniqueId) const;

			//returns a non-owning ptr to the resource
			//will load the resource if it has never been loaded before
			template<class T>
			T* get(UniqueId);

			using Mod = resources::mod;
			const Mod* getMod(UniqueId);

			//convert string to uid
			UniqueId getUid(std::string name);
			types::string as_string(UniqueId) const;

		private:
			void parseYaml(data::UniqueId, YAML::Node);
			void parseMod(std::string name, YAML::Node modRoot, std::function<bool(std::string)> dependency);

			//==parsing and loading data==
			std::unordered_map<std::string, resources::parserFunc> _resourceParsers;
			//==stored resource data==
			//list of used names
			std::unordered_set<std::string> _names;
			UniqueId _game;
			std::vector<UniqueId> _mods;
			//map of names to Uids
			std::unordered_map<std::string, UniqueId> _ids;
			//map of uids to resources
			property_bag<UniqueId> _resources;
			//list of unloaded resources
			std::vector<resources::resource_base*> _loadQueue;
		};
	}
}

#include "detail/data_manager.inl"

#endif // hades_data_hpp
