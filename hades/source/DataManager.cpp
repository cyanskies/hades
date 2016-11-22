#include "Hades/DataManager.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/files.hpp"
#include "Hades/Console.hpp"
#include "Hades/simple_resources.hpp"

namespace hades
{
	namespace data
	{
		void resource_base::load(data_manager* datamanager)
		{
			if (_resourceLoader)
			{
				_resourceLoader(this, datamanager);
				_generation++;
			}
		}

		data_manager::~data_manager()
		{}
		//application registers the custom resource types
		//parser must convert yaml into a resource manifest object
		void data_manager::register_resource_type(std::string name, data_manager::parserFunc parser)
		{
			_resourceParsers[name] = parser;
		}
		//loader reads manifest and loads data from disk
		void data_manager::register_resource_type(std::string name, data_manager::parserFunc parser, loaderFunc loader)
		{
			_resourceParsers[name] = parser;
			_resourceLoaders[name] = loader;
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
		}

		bool data_manager::loaded(std::string mod) const
		{
			//name hasn't even been used yet
			if (_names.find(mod) == _names.end())
				return false;

			try
			{
				auto r = get<resources::mod>(getUid(mod));
			}
			//name has been used, but not for a mod
			catch (std::runtime_error &e)
			{
				return false;
			}

			//name is used and is a mod
			return true;
		}

		//convert string to uid
		UniqueId data_manager::getUid(std::string name)
		{
			return _ids[name];
		}

		UniqueId data_manager::getUid(std::string name) const
		{
			auto i = _ids.find(name);
			if (i == _ids.end())
				throw std::runtime_error("Tried to get uniqueId that doesn't exist.");

			return i->second;
		}

		void data_manager::parseMod(std::string name, YAML::Node modRoot, std::function<bool(std::string)> dependency)
		{
			//read the mod header

			//for every other headers, check for a header parser
		}

	}

	DataManager::DataManager()
	{
		//register custom resource types
		register_resource_type("texture", resources::parseTexture, resources::loadTexture);
	}

	data::Resource<DataManager::Texture> DataManager::getTexture(data::UniqueId key) const
	{
		return get<Texture>(key);
	}
}