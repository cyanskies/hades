#include "Hades/simple_resources.hpp"

#include <array>

#include "yaml-cpp/yaml.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "Hades/Console.hpp"
#include "Hades/DataManager.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/files.hpp"

using namespace hades;

//posts a standard error message if the requested resource is of the wrong type
void resource_error(types::string resource_type, types::string resource_name, data::UniqueId mod)
{
	auto mod_ptr = data_manager->getMod(mod);
	LOGERROR("Name collision with identifier: " + resource_name + ", for " + resource_type + " while parsing mod: " 
		+ mod_ptr->name + ". Name has already been used for a different resource type.");
}

//posts error message if the yaml node is the wrong type
bool yaml_error(types::string resource_type, types::string resource_name,
	types::string property_name, types::string requested_type, data::UniqueId mod, bool test)
{
	if (!test)
	{
		auto mod_ptr = data_manager->getMod(mod);
		LOGERROR("Error parsing YAML, in mod: " + mod_ptr->name + ", type: " + resource_type + ", name: " 
			+ resource_name + ", for property: " + property_name + ". value must be " + requested_type);
	}
	
	return test;
}

namespace hades
{
	namespace resources
	{
		const size_t colour_count = 7;

		const std::array<sf::Color, colour_count> colours {
			sf::Color::Magenta,
			sf::Color::White,
			sf::Color::Red,
			sf::Color::Green,
			sf::Color::Blue,
			sf::Color::Yellow,
			sf::Color::Cyan
		};

		sf::Texture generate_checkerboard_texture(texture::size_type width, texture::size_type height, texture::size_type checker_scale,
			sf::Color c1, sf::Color c2)
		{
			std::vector<sf::Uint32> pixels(width * height);

			texture::size_type counter = 0;
			sf::Color c = c1;

			for (auto &p : pixels)
			{
				p = c.toInteger();
				if (counter++ % checker_scale == 0)
				{
					if (c == c1)
						c = c2;
					else
						c = c1;
				}
			}

			sf::Uint8* p = reinterpret_cast<sf::Uint8*>(pixels.data());

			sf::Image i;
			i.create(width, height, p);

			sf::Texture t;
			t.loadFromImage(i);

			return t;
		}

		//generates a group of 
		sf::Texture generate_default_texture(texture::size_type width = 32u, texture::size_type height = 32u)
		{
			static std::size_t counter = 0;

			auto t = generate_checkerboard_texture(width, height, 16, colours[counter++ % colour_count], sf::Color::Black);
			t.setRepeated(true);

			return t;
		}

		void parseTexture(data::UniqueId mod, YAML::Node& node, data::data_manager* dataman)
		{
			//default texture yaml
			//textures:
			//    default:
			//        width: 0 #0 = autodetect, no size checking will be done
			//        height: 0 
			//        source: #empty source, no file is specified, will always be default error texture
			//        smooth: false
			//        repeating: false
			//        mips: false

			const texture::size_type d_width = 0, d_height = 0;
			const std::string d_source;
			const bool d_smooth = false, d_repeating = false, d_mips = false;

			//for each node
			//{
			//	parse values
			//	load default error texture
			//	add to load queue
			//}

			for (auto n : node)
			{
				//get texture with this name if it has already been loaded
				//first node holds the maps name
				auto tnode = n.first;
				//second holds the map children
				auto tvariables = n.second;
				auto id = dataman->getUid(tnode.as<types::string>());
				texture* tex;

				if (!dataman->exists(id))
				{
					//resource doens't exist yet, create it
					auto texture_ptr = std::make_unique<texture>();
					tex = &*texture_ptr;
					dataman->set<texture>(id, std::move(texture_ptr));

					tex->height = d_height;
					tex->width = d_width;
					tex->smooth = d_smooth;
					tex->repeat = d_repeating;
					tex->mips = d_mips;
					tex->source = d_source;
					tex->id = id;
				}
				else
				{
					//retrieve it from the data store
					try
					{
						tex = dataman->get<texture>(id);
					}
					catch (data::resource_null&)
					{

					}
					catch (data::resource_wrong_type&)
					{
						//name is already used for something else, this cannnot be loaded
						auto name = n.as<types::string>();
						auto mod_ptr = dataman->getMod(mod);
						LOGERROR("Name collision with identifier: " + name + ", for texture while parsing mod: " + mod_ptr->name + ". Name has already been used for a different resource type.");
						//skip the rest of this loop and check the next node
						continue;
					}
				}

				tex->mod = mod;

				//only overwrite current values(or default if this is a new resource) if they are specified.
				auto width = tvariables["width"];
				if (!width.IsNull())
					tex->width = width.as<texture::size_type>(d_width);

				auto height = tvariables["height"];
				if (!height.IsNull())
					tex->height = height.as<texture::size_type>(d_height);

				
				auto smooth = tvariables["smooth"];
				if (!smooth.IsNull())
					tex->smooth = smooth.as<bool>(d_smooth);

				auto repeat = tvariables["repeating"];
				if (!repeat.IsNull())
					tex->repeat = repeat.as<bool>(d_repeating);

				auto mips = tvariables["mips"];
				if (!mips.IsNull())
					tex->mips = mips.as<bool>(d_mips);

				auto source = tvariables["source"];
				if (!source.IsNull())
					tex->source = source.as<types::string>(d_source);

				//if either size parameters are 0, then don't warn for size mismatch
				if (tex->width == 0 || tex->height == 0)
					tex->width = tex->height = 0;

				if (tex->width == 0)
					tex->value = generate_default_texture();
				else
					tex->value = generate_default_texture(tex->width, tex->height);

				dataman->refresh(id);
			}
		}

		void loadTexture(resources::resource_base* r, data::data_manager* dataman)
		{
			auto tex = static_cast<texture*>(r);
			
			if (!tex->source.empty())
			{
				//the mod not being available should be 'impossible'
				auto mod = dataman->getMod(tex->mod);
				
				try
				{
					files::FileStream fstream(mod->source, tex->source);
					tex->value.loadFromStream(fstream);
				}
				catch (files::file_exception e)
				{
					LOGERROR("Failed to load texture: " + mod->source + "/" + tex->source + ". " + e.what());
				}

				//if the width or height are 0, then don't warn about size mismatch
				//otherwise log unexpected size
				auto size = tex->value.getSize();
				if (tex->width == 0 &&
					size.x != tex->width || size.y != tex->height)
				{
					LOGWARNING("Loaded texture: " + mod->source + "/" + tex->source + ". Texture size different from requested. Requested(" +
						std::to_string(tex->width) + ", " + std::to_string(tex->height) + "), Found(" + std::to_string(size.x) + ", " + std::to_string(size.y) + ")");
				}
			}
		}

		void parseString(data::UniqueId mod, YAML::Node& node, data::data_manager* dataman)
		{
			//strings yaml
			//strings: 
			//    id: value
			//    id2: value2

			for (auto n : node)
			{
				//get string with this name if it has already been loaded
				//first node holds the maps name
				auto tnode = n.first;
				//second holds the map children
				auto string = n.second.as<types::string>();
				auto id = dataman->getUid(tnode.as<types::string>());

				auto string_ptr = std::make_unique<resources::string>();

				string_ptr->mod = mod;

				auto moddata = dataman->get<resources::mod>(mod);

				string_ptr->source = moddata->source;
				string_ptr->id = id;
				string_ptr->value = string;

				dataman->set<resources::string>(id, std::move(string_ptr));
			}
		}

		void parseSystem(data::UniqueId mod, YAML::Node& node, data::data_manager*)
		{
			assert(false && "mods cannot create systems untill scripting is introduced.");
		}

		void loadSystem(resource_base* r, data::data_manager* dataman)
		{
			//same as above
			return;
		}

		CurveType readCurveType(types::string s)
		{
			if (s == "const")
				return CurveType::CONST;
			else if (s == "step")
				return CurveType::STEP;
			else if (s == "linear")
				return CurveType::LINEAR;
			else if (s == "pulse")
				return CurveType::PULSE;
			else
				return CurveType::ERROR;
		}

		VariableType readVariableType(types::string s)
		{
			if (s == "int")
				return VariableType::INT;
			else if (s == "float")
				return VariableType::FLOAT;
			else if (s == "bool")
				return VariableType::BOOL;
			else if (s == "string")
				return VariableType::STRING;
			else if (s == "int_vector")
				return VariableType::VECTOR_INT;
			else if (s == "float_vector")
				return VariableType::VECTOR_FLOAT;
			else
				return VariableType::ERROR;
		}


		void parseCurve(data::UniqueId mod, YAML::Node& node, data::data_manager* dataman)
		{
			//curves:
			//		name:
			//			type: default: step
			//			value: default: int32
			//			sync: default: false
			//			save: default false

			//these are loaded into the game instance before anything else
			//curves cannot change type/value once they have been created,
			//the resource is used to created synced curves of the correct type
			//on clients
			//a blast of names to VariableIds must be sent on client connection
			//so that we can refer to curves by id instead of by string
			
			//first check that our node is valid
			//no point looping though if their are not children
			static const types::string resource_type = "curve";
			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			//then look through the children
			for (auto n : node)
			{
				//if this entry isn't a map, then it cannot have any child values
				if (!yaml_error(resource_type, n.as<types::string>(), "n/a", "map", mod, n.IsMap()))
					continue;

				//maps have a first and second value
				//the first is the string label of the entry
				//the second is the stored node
				auto tnode = n.first;
				auto curveInfo = n.second;

				//record this nodes name
				const types::string name = tnode.as<types::string>();

				auto id = dataman->getUid(name);
				curve* c;

				//try to get this
				if (!dataman->exists(id))
				{
					auto curve_ptr = std::make_unique<curve>();
					c = &*curve_ptr;
					dataman->set<curve>(id, std::move(curve_ptr));

					c->curve_type = CurveType::STEP;
					c->data_type = VariableType::INT;
					c->save = false;
					c->sync = false;
					c->id = id;
				}
				else
				{
					try
					{
						c = dataman->get<curve>(id);
					}
					catch (data::resource_wrong_type&)
					{
						//name is already used for something else, this cannnot be loaded
						resource_error(resource_type, name, mod);
						//skip the rest of this loop and check the next node
						continue;
					}
				}

				c->mod = mod;

				//checking a key will return the value accociated with it
				//eg
				//thisnode:
				//	type: curve_type

				//test that the type is a scalar, and not a container or map
				auto type = curveInfo["type"];
				if (!type.IsNull() && yaml_error(resource_type, name, "type", "scalar", mod, type.IsScalar()))
					c->curve_type = readCurveType(type.as<types::string>());

				auto var = curveInfo["value"];
				if (!var.IsNull() && yaml_error(resource_type, name, "value", "scalar", mod, var.IsScalar()))
					c->data_type = readVariableType(var.as<types::string>());

				auto sync = curveInfo["sync"];
				if (!sync.IsNull() && yaml_error(resource_type, name, "sync", "scalar", mod, sync.IsScalar()))
					c->sync = sync.as<bool>();

				auto save = curveInfo["save"];
				if (!save.IsNull() && yaml_error(resource_type, name, "save", "scalar", mod, save.IsScalar()))
					c->save = save.as<bool>();
			}
		}

		void parseAnimation(data::UniqueId mod, YAML::Node& node, data::data_manager* data)
		{
			//animations:
			//	example-animation:
			//		duration: 1ms
			//		texture: example-texture
			//		size: [w, h]
			//		frames:
			//			- [x, y, d]
			//			- [x, y, d]


			for (auto n : node)
			{
				auto node = n.first;
				auto animation_node = n.second;
				auto id = data->getUid(node.as<types::string>());
				animation* a;

				static const types::string resource_type = "animation";
				const types::string name = n.as<types::string>();

				if (!data->exists(id))
				{
					auto animation_ptr = std::make_unique<animation>();
					a = &*animation_ptr;
					data->set<animation>(id, std::move(animation_ptr));

					a->duration = 1;
					a->id = id;
					a->source = types::string();
					a->texture = data::UniqueId::Zero;
					a->value = std::vector<animation_frame>(0);
				}
				else
				{
					try
					{
						a = data->get<animation>(id);
					}
					catch (data::resource_wrong_type&)
					{
						//name is already used for something else, this cannnot be loaded
						resource_error(resource_type, name, mod);
						//skip the rest of this loop and check the next node
						continue;
					}
				}

				a->mod = mod;
				
				auto duration = animation_node["duration"];
				if (!duration.IsNull() && yaml_error(resource_type, name, "duration", "scalar", mod, duration.IsScalar()))
					a->duration = duration.as<types::uint32>();
			}
		}
	}
}