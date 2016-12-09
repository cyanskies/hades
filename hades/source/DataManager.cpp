#include "Hades/DataManager.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/files.hpp"
#include "Hades/Console.hpp"
#include "Hades/simple_resources.hpp"

namespace hades
{
	namespace data
	{
		data_manager::~data_manager()
		{}
		//application registers the custom resource types
		//parser must convert yaml into a resource manifest object
		void data_manager::register_resource_type(std::string name, data_manager::parserFunc parser)
		{
			_resourceParsers[name] = parser;
		}

		//game is the name of a folder or archive containing a game.yaml file
		void data_manager::load_game(std::string game)
		{
			static bool game_loaded = false;

			if (game_loaded)
			{
				LOG("Tried to load" + game + ", Already loaded a game, skipping");
				return;//game already loaded ignore
			}

			try
			{
				add_mod(game, true, "game.yaml");
				game_loaded = true;
			}
			catch (YAML::Exception &e)
			{
				LOGERROR(e.what());
			}
		}

		//mod is the name of a folder or archive containing a mod.yaml file
		void data_manager::add_mod(std::string mod, bool autoLoad, std::string name)
		{
			std::string modyaml;
			try
			{
				modyaml = files::as_string(mod, name);
			}
			catch (files::file_exception &f)
			{
				LOGERROR(f.what());
				return;
			}

			//parse game.yaml
			auto root = YAML::Load(modyaml.c_str());
			if(autoLoad)
				parseMod(mod, root, [this](std::string s) {this->add_mod(s, true); return true;});
			else
				parseMod(mod, root, std::bind(&data_manager::loaded, this, std::placeholders::_1));

			//record the list of loaded games/mods
			if (name == "game.yaml")
				_game = getUid(mod);
			else
				_mods.push_back(getUid(mod));
		}

		bool data_manager::loaded(std::string mod)
		{
			//name hasn't even been used yet
			if (_names.find(mod) == _names.end())
				return false;

			try
			{
				auto r = get<resources::mod>(getUid(mod));
			}
			//name has been used, but not for a mod
			catch (std::runtime_error&)
			{
				return false;
			}

			//name is used and is a mod
			return true;
		}

		data_manager::Mod* data_manager::getMod(UniqueId mod)
		{
			return get<resources::mod>(mod);
		}

		//convert string to uid
		UniqueId data_manager::getUid(std::string name)
		{
			return _ids[name];
		}

		types::string data_manager::as_string(UniqueId uid) const
		{
			for (auto id : _ids)
			{
				if (id.second == uid)
					return id.first;
			}

			return "ERROR_NO_UNIQUE_ID";
		}

		void data_manager::parseYaml(data::UniqueId mod, YAML::Node root)
		{
			//loop though each of the root level nodes and pass them off
			//to specialised handlers if present
			for (auto header : root)
			{
				//don't try to read mod headers
				if (header.first.as<types::string>() == "mod")
					continue;

				auto type = header.first.as<types::string>();

				//if type is include, then open the file as pass it to parseyaml
				//TODO:

				//if this resource name has a parser then load it
				auto parser = _resourceParsers[type];
				if (parser)
					parser(mod, header.second, this);
			}
		}

		void data_manager::parseMod(std::string source, YAML::Node modRoot, std::function<bool(std::string)> dependencycheck)
		{
			auto modKey = getUid(source);

			//check that the mod isn't already loaded;
			//if it is then warn and overwrite
			//TODO: 


			//read the mod header
			auto mod = modRoot["mod"];
			if (mod.IsNull())
			{
				//missing mod header
				LOGERROR("mod header missing for mod: " + source);
				return;
			}

			//parse the mod header;
			auto mod_data = std::make_unique<resources::mod>();
			mod_data->source = source;
			auto mod_name = mod["name"];
			if (mod_name.IsNull())
			{
				//missing mod name
				LOGERROR("name missing for mod: " + source);
				return;
			}

			auto name = mod_name.as<types::string>();
			mod_data->name = name;

			//check mod dependencies
			//TODO:

			//store the data
			set<resources::mod>(modKey, std::move(mod_data));

			//for every other headers, check for a header parser
			parseYaml(modKey, modRoot);

			LOG("Loaded mod: " + name);
		}
	}

	DataManager::DataManager()
	{
		//register custom resource types
		register_resource_type("textures", resources::parseTexture);
		register_resource_type("strings", resources::parseString);
	}

	DataManager::Texture* DataManager::getTexture(data::UniqueId key)
	{
		return get<Texture>(key);
	}

	DataManager::String* DataManager::getString(data::UniqueId key)
	{
		return get<String>(key);
	}
}