#include "Hades/simple_resources.hpp"

#include <array>

#include "yaml-cpp/yaml.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "Hades/Console.hpp"
#include "Hades/DataManager.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/files.hpp"
#include "Hades/resource/fonts.hpp"

using namespace hades;

//posts a standard error message if the requested resource is of the wrong type
void resource_error(types::string resource_type, types::string resource_name, data::UniqueId mod)
{
	auto mod_ptr = data_manager->getMod(mod);
	LOGERROR("Name collision with identifier: " + resource_name + ", for " + resource_type + " while parsing mod: " 
		+ mod_ptr->name + ". Name has already been used for a different resource type.");
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

			const types::string d_source, resource_type = "textures";

			for (auto n : node)
			{
				//get texture with this name if it has already been loaded
				//first node holds the maps name
				auto tnode = n.first;
				auto name = tnode.as<types::string>();
				//second holds the map children
				auto tvariables = n.second;
				auto id = dataman->getUid(tnode.as<types::string>());
				auto tex = data::FindOrCreate<texture>(id, mod, dataman);

				if (!tex)
					continue;

				tex->width = yaml_get_scalar(tvariables, resource_type, name, "width", mod, tex->width);
				tex->height = yaml_get_scalar(tvariables, resource_type, name, "height", mod, tex->height);
				tex->smooth = yaml_get_scalar(tvariables, resource_type, name, "smooth", mod, tex->smooth);
				tex->repeat = yaml_get_scalar(tvariables, resource_type, name, "repeating", mod, tex->repeat);
				tex->mips = yaml_get_scalar(tvariables, resource_type, name, "mips", mod, tex->mips);
				tex->source = yaml_get_scalar(tvariables, resource_type, name, "source", mod, tex->source);

				//if either size parameters are 0, then don't warn for size mismatch
				if (tex->width == 0 || tex->height == 0)
					tex->width = tex->height = 0;

				if (tex->width == 0)
					tex->value = generate_default_texture();
				else
					tex->value = generate_default_texture(tex->width, tex->height);
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
					files::FileStream fstream = files::make_stream(mod->source, tex->source);
					tex->value.loadFromStream(fstream);
					tex->value.setSmooth(tex->smooth);
					tex->value.setRepeated(tex->repeat);

					if (tex->mips && !tex->value.generateMipmap())
						LOGWARNING("Failed to generate MipMap for texture: " + hades::data_manager->as_string(tex->id));
				}
				catch (files::file_exception e)
				{
					LOGERROR("Failed to load texture: " + mod->source + "/" + tex->source + ". " + e.what());
				}

				//if the width or height are 0, then don't warn about size mismatch
				//otherwise log unexpected size
				auto size = tex->value.getSize();
				if (tex->width != 0 &&
					(size.x != tex->width || size.y != tex->height))
				{
					LOGWARNING("Loaded texture: " + mod->source + "/" + tex->source + ". Texture size different from requested. Requested(" +
						std::to_string(tex->width) + ", " + std::to_string(tex->height) + "), Found(" + std::to_string(size.x) + ", " + std::to_string(size.y) + ")");
					//NOTE: if the texture is the wrong size
					// then enable repeating, to avoid leaving
					// gaps in the world(between tiles and other such stuff).
					tex->value.setRepeated(true);
				}

				tex->loaded = true;
			}
		}

		void parseString(data::UniqueId mod, YAML::Node& node, data::data_manager* dataman)
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
				auto id = dataman->getUid(tnode.as<types::string>());

				auto str = data::FindOrCreate<resources::string>(id, mod, dataman);// = std::make_unique<resources::string>();
				
				if (!str)
					continue;

				str->mod = mod;
				auto moddata = dataman->get<resources::mod>(mod);
				str->source = moddata->source;
				str->value = string;
			}
		}

		void parseSystem(data::UniqueId mod, YAML::Node& node, data::data_manager*)
		{
			assert(false && "mods cannot create systems until scripting is introduced.");
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
			//			type: default: step //determines how the values are read between keyframes
			//			value: default: int32 //determines the value type
			//			sync: default: false //true if this should be syncronised to the client
			//			save: default false //true if this should be saved when creating a save file

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
				if (!yaml_error(resource_type, n.first.as<types::string>(), "n/a", "map", mod, n.second.IsMap()))
					continue;

				//maps have a first and second value
				//the first is the string label of the entry
				//the second is the stored node
				auto tnode = n.first;
				auto curveInfo = n.second;

				//record this nodes name
				const types::string name = tnode.as<types::string>();

				auto id = dataman->getUid(name);
				curve* c = hades::data::FindOrCreate<curve>(id, mod, dataman);

				if (!c)
					continue;

				//checking a key will return the value accociated with it
				//eg
				//thisnode:
				//	type: curve_type

				//test that the type is a scalar, and not a container or map
				auto type = curveInfo["type"];
				if (type.IsDefined() && yaml_error(resource_type, name, "type", "scalar", mod, type.IsScalar()))
					c->curve_type = readCurveType(type.as<types::string>());

				auto var = curveInfo["value"];
				if (var.IsDefined() && yaml_error(resource_type, name, "value", "scalar", mod, var.IsScalar()))
					c->data_type = readVariableType(var.as<types::string>());

				auto sync = curveInfo["sync"];
				if (sync.IsDefined() && yaml_error(resource_type, name, "sync", "scalar", mod, sync.IsScalar()))
					c->sync = sync.as<bool>();

				auto save = curveInfo["save"];
				if (save.IsDefined() && yaml_error(resource_type, name, "save", "scalar", mod, save.IsScalar()))
					c->save = save.as<bool>();
			}
		}

		void parseAnimation(data::UniqueId mod, YAML::Node& node, data::data_manager* data)
		{
			//animations:
			//	example-animation:
			//		duration: 1.0s
			//		texture: example-texture
			//		size: [w, h]
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
				auto id = data->getUid(node.as<types::string>());
				animation* a = data::FindOrCreate<animation>(id, mod, data);

				if (!a)
					continue;

				const types::string name = n.as<types::string>();

				a->duration = yaml_get_scalar(animation_node, resource_type, name, "duration", mod, a->duration);

				auto texture_str = animation_node["texture"];
				if (texture_str.IsDefined() && yaml_error(resource_type, name, "texture", "scalar", mod, texture_str.IsScalar()))
					a->tex = data_manager->getTexture(data_manager->getUid(texture_str.as<types::string>()));

				a->width = yaml_get_scalar(animation_node, resource_type, name, "width", mod, a->width);
				a->height = yaml_get_scalar(animation_node, resource_type, name, "height", mod, a->height);

				//now get all the frames
				auto frames = animation_node["frames"];
				if (frames.IsDefined() && yaml_error(resource_type, name, "frames", "sequence", mod, frames.IsSequence()))
				{
					std::vector<animation_frame> frame_vector;

					bool bad_frames = false;
					types::uint16 frame_count = 0;
					//frames is a sequence node
					for (auto &frame : frames)
					{
						frame_count++;
						if (!yaml_error(resource_type, name, "frame", "sequence", mod, frame.IsSequence()))
						{
							bad_frames = true;
							break;
						}
						else if(!yaml_error(resource_type, name, "frame", "[uint16, uint16, float]", mod, frame.size() == 3))
						{
							bad_frames = true;
							break;
						}

						frame_vector.emplace_back( frame[0].as<types::uint16>(),	// X
							frame[1].as<types::uint16>(),							// Y
							frame[3].as<float>() );									// duration
						
						auto &f = frame_vector.back();
						//check that the x and y values are within the limits of int32::max
						if (f.x + a->width > a->tex->width)
						{
							bad_frames = true;
							LOGERROR("animation rectangle would be outside the texture. For animation: " + name + ", frame number: " + std::to_string(frame_count));
							break;
						}
						else if (f.y + a->height > a->tex->height)
						{
							bad_frames = true;
							LOGERROR("animation rectangle would be outside the texture. For animation: " + name + ", frame number: " + std::to_string(frame_count));
							break;
						}
					}//for frame in frames

					if (bad_frames)
					{
						LOGERROR("Animation had frames which contained an error. For animation: " + name + ", This animation will not be valid");
						a->value.clear();
						continue;
					}

					//normalise all the frame durations
					float sum = 0.f;
					for (auto &f : frame_vector)
						sum += f.duration;

					assert(sum != 0.f);
					for (auto &f : frame_vector)
						f.duration /= sum;

					a->value = std::move(frame_vector);
				}//if frames not null
			}//for animations
		}//parse animations

		void loadFont(resource_base* r, data::data_manager* data)
		{
			assert(dynamic_cast<font*>(r));
			auto f = static_cast<font*>(r);
			auto mod = data->getMod(f->mod);

			try
			{
				files::FileStream fstream = files::make_stream(mod->source, f->source);
				f->value.loadFromStream(fstream);
			}
			catch (files::file_exception e)
			{
				LOGERROR("Failed to load font: " + mod->source + "/" + f->source + ". " + e.what());
			}
		}

		void parseFont(data::UniqueId mod, YAML::Node& node, data::data_manager* data)
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
				auto id = data->getUid(tnode.as<types::string>());
				auto name = tnode.as<types::string>();
				font *f = data::FindOrCreate<font>(id, mod, data);
				
				if (!f)
					continue;
				
				f->source = source;
			}
		}
	}//namespace resources
}//namespace hades
