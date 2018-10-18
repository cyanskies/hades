#include "Hades/simple_resources.hpp"

#include <array>

#include "yaml-cpp/yaml.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "Hades/Console.hpp"
#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"
#include "Hades/files.hpp"
#include "Hades/resource/fonts.hpp"
#include "hades/utility.hpp"

#include "Hades/curve_extra.hpp"
#include "hades/parser.hpp"
#include "hades/texture.hpp"
#include "hades/yaml_parser.hpp"

using namespace hades;

//posts a standard error message if the requested resource is of the wrong type
void resource_error(types::string resource_type, types::string resource_name, unique_id mod)
{
	auto mod_ptr = data::get<resources::mod>(mod);
	LOGERROR("Name collision with identifier: " + resource_name + ", for " + resource_type + " while parsing mod: "
		+ mod_ptr->name + ". Name has already been used for a different resource type.");
}

namespace hades
{
	namespace resources
	{
		void parseString(unique_id mod, const YAML::Node& node, data::data_manager*);
	}

	void RegisterCommonResources(hades::data::data_system *data)
	{
		data->register_resource_type("strings", resources::parseString);
	}

	namespace resources
	{	
		void parseString(unique_id mod, const YAML::Node& node, data::data_manager* dataman)
		{
			//strings yaml
			//strings:
			//    id: value
			//    id2: value2

			types::string resource_type = "strings";
			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto n : node)
			{
				//get string with this name if it has already been loaded
				//first node holds the maps name
				auto tnode = n.first;
				auto name = tnode.as<types::string>();

				//second holds the map children
				auto string = n.second.as<types::string>();
				auto id = dataman->get_uid(tnode.as<types::string>());

				auto str = data::FindOrCreate<resources::string>(id, mod, dataman);// = std::make_unique<resources::string>();

				if (!str)
					continue;

				str->mod = mod;
				auto moddata = dataman->get<resources::mod>(mod);
				str->source = moddata->source;
				str->value = string;
			}
		}
	}//namespace resources
}//namespace hades
