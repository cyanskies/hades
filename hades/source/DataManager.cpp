#include "Hades/DataManager.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/archive.hpp"
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
			if(!zip::file_exists(game, "game.yaml"))
				throw std::runtime_error(game + "doesn't contain a game.yaml");

			//open ./game.* as archive
			auto gameyaml = zip::read_text_from_archive(game, "game.yaml");

			//parse game.yaml
			auto root = YAML::Load(gameyaml.c_str());
			parseMod(game, root, [this](std::string s) {this->add_mod(s, true); return true;});
		}

		//mod is the name of a folder or archive containing a mod.yaml file
		void data_manager::add_mod(std::string mod, bool autoLoad)
		{
			if (!zip::file_exists(mod, "mod.yaml"))
				throw std::runtime_error("Failed to load requested mod");

			//open ./game.* as archive
			auto game = zip::read_text_from_archive(mod, "mod.yaml");

			//parse game.yaml
			auto root = YAML::Load(game.c_str());
			if(autoLoad)
				parseMod(game, root, [this](std::string s) {this->add_mod(s, true); return true;});
			else
				parseMod(game, root, std::bind(&data_manager::loaded, this, std::placeholders::_1));
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
			catch (std::runtime_error *e)
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
			;
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