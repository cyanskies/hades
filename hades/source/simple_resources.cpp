#include "Hades/simple_resources.hpp"

#include <array>

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "hades/Console.hpp"
#include "hades/Data.hpp"
#include "hades/data_system.hpp"
#include "hades/files.hpp"
#include "hades/font.hpp"
#include "hades/resource/fonts.hpp"
#include "hades/utility.hpp"

#include "hades/curve_extra.hpp"
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
		void parseString(unique_id mod, const data::parser_node &node, data::data_manager&);
	}

	void RegisterCommonResources(hades::data::data_system *data)
	{
		data->register_resource_type("strings", resources::parseString);

		//create default font
		//we do it here because the resource isn't availble yet
		//in hades-core
		//TODO: move to hades core
		auto f = data->find_or_create<resources::font>(resources::default_font_id(), unique_id::zero);
		assert(f);

		f->loaded = f->value.loadFromMemory(default_font.data(), default_font.size());
	}

	namespace resources
	{	
		void parseString(unique_id mod, const data::parser_node &n, data::data_manager &d)
		{
			//strings yaml
			//strings:
			//    id: value
			//    id2: value2

			//const auto resource_type = "strings";
			
			const auto strings = n.get_children();

			for (const auto &s : strings)
			{
				//get string with this name if it has already been loaded
				//first node holds the maps name
				const auto name = s->to_string();
				const auto id = d.get_uid(name);
				
				auto str = d.find_or_create<resources::string>(id, mod);
				if (!str)
					continue;

				const auto mod_info = d.get<resources::mod>(mod);

				const auto value = s->get_child();
				const auto string = value->to_string();

				str->source = mod_info->source;
				str->value = string;
			}
		}
	}//namespace resources
}//namespace hades
