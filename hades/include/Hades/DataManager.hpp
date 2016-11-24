#ifndef HADES_DATAMANAGER_HPP
#define HADES_DATAMANAGER_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SFML/Graphics/Texture.hpp"

#include "archive.hpp"
#include "type_erasure.hpp"
#include "Types.hpp"
#include "Hades/UniqueId.hpp"

namespace YAML
{
	class Node;
}

namespace hades
{
	namespace data
	{
		class data_manager;
	}

	namespace resources
	{
		template<class T>
		struct resource_type
		{
			using loaderFunc = std::function<void(resource_type<T>*, data::data_manager*)>;

			virtual ~resource_type() {}

			void load(data::data_manager*);
			
			//the file this resource should be loaded from
			//the name of the mod for resources that are parsed and not loaded
			std::string source;
			//the actual resource
			T value;
			//the mod that the resource was most recently specified in
			//not nessicarily the only mod to specify this resource.
			data::UniqueId mod;
		private:
			loaderFunc _resourceLoader;
		};

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

		//TODO: move these in another header
		struct string : public resource_type<std::string>
		{};

		struct texture : public resource_type<sf::Texture>
		{
			//max texture size is 512
			types::uint8 width, height;
			bool smooth, repeat;
		};
	}

	namespace data
	{
		class data_manager
		{
		public:

			using parserFunc = std::function<void(UniqueId mod, YAML::Node& node, data_manager*)>;

			virtual ~data_manager();
			//application registers the custom resource types
			//parser must convert yaml into a resource manifest object
			void register_resource_type(std::string name, parserFunc parser);
	
			//game is the name of a folder or archive containing a game.yaml file
			void load_game(std::string game);
			//mod is the name of a folder or archive containing a mod.yaml file
			void add_mod(std::string mod, bool autoLoad = false, std::string name = "mod.yaml");

			//returns true if the mod or game with that name has been parsed
			bool loaded(std::string mod) const;

			//loads any queued resources
			void load_resources();

			template<class T>
			void set(UniqueId, T);

			//returns a non-owning ptr to the resource
			template<class T>
			T const *get(UniqueId) const;

			using Mod = resources::mod;

			Mod const *getMod(UniqueId) const;

			//convert string to uid
			UniqueId getUid(std::string name);
			UniqueId getUid(std::string name) const;

		private:
			void parseMod(std::string name, YAML::Node modRoot, std::function<bool(std::string)> dependency);

			//==parsing and loading data==
			std::unordered_map<std::string, parserFunc> _resourceParsers;
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
			std::vector<resource_base*> _loadQueue;
		};
	}

	//default hades datamanager
	//registers that following resource types:
	//texture
	//sound
	//document
	//string store
	class DataManager : public data::data_manager
	{
	public:
		DataManager();

		using Texture = resources::texture;
		Texture const * getTexture(data::UniqueId) const;
	};
}

#include "detail/DataManager.inl"

#endif // hades_data_hpp
