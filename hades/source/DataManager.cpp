#include "Hades/DataManager.hpp"

namespace hades
{
	namespace data
	{
		void resource_base::load(data_manager* datamanager)
		{
			if(_resourceLoader && _resourceLoader(this, datamanager))
				_generation++;
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
			
		}

		//mod is the name of a folder or archive containing a mod.yaml file
		void data_manager::add_mod(std::string mod)
		{}

		//register a new uid
		UniqueId data_manager::createUid(std::string name)
		{
			return UniqueId(0);
		}
		//convert string to uid
		UniqueId data_manager::getUid(std::string name)
		{
			return UniqueId(0);
		}
	}
}