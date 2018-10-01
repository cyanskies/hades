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
		void parseAnimation(unique_id mod, const YAML::Node& node, data::data_manager*);
		void loadAnimation(resource_base* r, data::data_manager* dataman);
		void loadFont(resource_base* r, data::data_manager* data);
		void parseFont(unique_id mod, const YAML::Node& node, data::data_manager*);
	}

	void RegisterCommonResources(hades::data::data_system *data)
	{
		register_texture_resource(*data);
		register_curve_resource(*data);
		//data->register_resource_type("actions", nullptr);
		data->register_resource_type("animations", resources::parseAnimation);
		data->register_resource_type("fonts", resources::parseFont);
		data->register_resource_type("strings", resources::parseString);
	}

	namespace resources
	{
		font::font() : resource_type<sf::Font>(loadFont) {}
		animation::animation() : resource_type<std::vector<animation_frame>>(loadAnimation) {}

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

		//throws runtime error
		animation_frame parseFrame(const YAML::Node &node)
		{
			if (!node.IsSequence())
				throw std::runtime_error("Expected frame to be a sequence of values");
			else if (node.size() != 3)
				throw std::runtime_error("expected frame to hold exactly 3 values in the form of [int, int, float]");

			try
			{
				animation_frame out = { node[0].as<types::uint16>(),	// X
					node[1].as<types::uint16>(),						// Y
					node[2].as<float>() };							// duration

				return out;
			}
			catch (YAML::Exception &e)
			{
				auto message = "error parsing frame: " + e.msg + ", at line: " + to_string(e.mark.line) +
					", coloumn: " +	to_string(e.mark.column) + ", pos: " + to_string(e.mark.pos);
				throw std::runtime_error(message);
			}			
		}

		//throws runtime erorr
		std::vector<animation_frame> parseFrames(unique_id mod, const YAML::Node &node)
		{
			std::vector<animation_frame> frame_vector;

			types::uint16 frame_count = 0;
			
			if(!node.IsSequence())
				throw std::runtime_error("Frames must contain one or more frame nodes");

			//if the nodes chidren are also sequences
			//then we probably contain valid frames
			if (node[0].IsSequence())
			{
				//multiple frames
				for (const auto &frame : node)
				{
					frame_vector.emplace_back(parseFrame(frame));
					++frame_count;
				}
			}
			else //otherwise we must have a single frame(or an invalid node)
			{
				frame_vector.emplace_back(parseFrame(node));
				frame_count = 1;
			}

			//normalise all the frame durations
			float sum = 0.f;
			for (auto &f : frame_vector)
				sum += f.duration;

			assert(sum != 0.f);
			for (auto &f : frame_vector)
				f.duration /= sum;

			return frame_vector;
		}

		void parseAnimation(unique_id mod, const YAML::Node& node, data::data_manager* data)
		{
			//animations:
			//	example-animation:
			//		duration: 1.0s
			//		texture: example-texture
			//		width: +int
			//      height: +int
			//		frames:
			//			- [x, y, d]
			//			- [x, y, d]

			static const types::string resource_type = "animation";
			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto n : node)
			{
				if (!yaml_error(resource_type, n.first.as<types::string>(), "n/a", "map", mod, n.second.IsMap()))
					continue;

				auto node = n.first;
				auto animation_node = n.second;
				const types::string name = node.as<types::string>();
				auto id = data->get_uid(name);
				animation* a = data::FindOrCreate<animation>(id, mod, data);

				if (!a)
					continue;

				a->duration = yaml_get_scalar(animation_node, resource_type, name, "duration", mod, a->duration);

				auto texture_str = yaml_get_scalar<types::string>(animation_node, resource_type, name, "texture", mod, "");
				if (texture_str.empty())
				{
					LOGWARNING("Animation: " + name + ", is missing a texture");
					continue;
				}

				a->tex = data::FindOrCreate<resources::texture>(data::get_uid(texture_str), mod, data);
				a->width = yaml_get_scalar(animation_node, resource_type, name, "width", mod, a->width);
				a->height = yaml_get_scalar(animation_node, resource_type, name, "height", mod, a->height);

				//now get all the frames
				auto frames = animation_node["frames"];
				if (frames.IsDefined())
				{
					try
					{
						a->value = parseFrames(mod, frames);
					}
					catch (std::runtime_error &e)
					{
						LOGERROR("Failed to parse frames for animation: " + name + ", reason:");
						LOGERROR(e.what());
						continue;
					}
				}
			}//for animations
		}//parse animations

		void loadAnimation(resource_base* r, data::data_manager* data)
		{
			assert(dynamic_cast<animation*>(r));
			auto a = static_cast<animation*>(r);
			if (!a->tex->loaded)
				//data->get will lazy load texture
				const auto texture = data->get<resources::texture>(a->tex->id);
		}

		void loadFont(resource_base* r, data::data_manager* data)
		{
			assert(dynamic_cast<font*>(r));
			auto f = static_cast<font*>(r);
			auto mod = data->get<resources::mod>(f->mod);

			try
			{
				auto fstream = files::make_stream(mod->source, f->source);
				f->value.loadFromStream(fstream);
			}
			catch (const files::file_exception &e)
			{
				LOGERROR("Failed to load font: " + mod->source + "/" + f->source + ". " + e.what());
			}
		}

		void parseFont(unique_id mod, const YAML::Node& node, data::data_manager* data)
		{
			//fonts yaml
			//fonts:
			//    name: source
			//    name2: source2

			types::string resource_type = "fonts";
			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto n : node)
			{
				//get string with this name if it has already been loaded
				//first node holds the maps name
				auto tnode = n.first;
				//second holds the map children
				auto source = n.second.as<types::string>();
				auto id = data->get_uid(tnode.as<types::string>());
				auto name = tnode.as<types::string>();
				font *f = data::FindOrCreate<font>(id, mod, data);

				if (!f)
					continue;

				f->source = source;
			}
		}
	}//namespace resources
}//namespace hades
