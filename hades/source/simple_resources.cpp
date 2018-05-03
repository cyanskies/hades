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

using namespace hades;

//posts a standard error message if the requested resource is of the wrong type
void resource_error(types::string resource_type, types::string resource_name, unique_id mod)
{
	auto mod_ptr = data::Get<resources::mod>(mod);
	LOGERROR("Name collision with identifier: " + resource_name + ", for " + resource_type + " while parsing mod: "
		+ mod_ptr->name + ". Name has already been used for a different resource type.");
}

namespace hades
{
	namespace resources
	{
		void parseTexture(unique_id mod, const YAML::Node& node, data::data_manager*);
		void loadTexture(resource_base* r, data::data_manager* dataman);
		void parseString(unique_id mod, const YAML::Node& node, data::data_manager*);
		void parseSystem(unique_id mod, const YAML::Node& node, data::data_manager*);
		void loadSystem(resource_base* r, data::data_manager* dataman);
		void parseCurve(unique_id mod, const YAML::Node& node, data::data_manager*);
		void parseAnimation(unique_id mod, const YAML::Node& node, data::data_manager*);
		void loadAnimation(resource_base* r, data::data_manager* dataman);
		void loadFont(resource_base* r, data::data_manager* data);
		void parseFont(unique_id mod, const YAML::Node& node, data::data_manager*);
	}

	void RegisterCommonResources(hades::data::data_system *data)
	{
		data->register_resource_type("actions", nullptr);
		data->register_resource_type("animations", resources::parseAnimation);
		data->register_resource_type("curves", resources::parseCurve);
		data->register_resource_type("fonts", resources::parseFont);
		data->register_resource_type("strings", resources::parseString);
		data->register_resource_type("systems", resources::parseSystem);
		data->register_resource_type("textures", resources::parseTexture);

		const auto max_tex_size = sf::Texture::getMaximumSize();
		const auto max_engine_size = std::numeric_limits<resources::texture::size_type>::max();
		//TODO warn if tex_size < engine_size
	}

	namespace resources
	{
		texture::texture() : resource_type<sf::Texture>(loadTexture) {}
		font::font() : resource_type<sf::Font>(loadFont) {}
		animation::animation() : resource_type<std::vector<animation_frame>>(loadAnimation) {}

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

		void parseTexture(unique_id mod, const YAML::Node& node, data::data_manager* dataman)
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
				auto mod = dataman->get<resources::mod>(tex->mod);

				try
				{
					auto fstream = files::make_stream(mod->source, tex->source);
					tex->value.loadFromStream(fstream);
					tex->value.setSmooth(tex->smooth);
					tex->value.setRepeated(tex->repeat);

					if (tex->mips && !tex->value.generateMipmap())
						LOGWARNING("Failed to generate MipMap for texture: " + dataman->getAsString(tex->id));
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

		void parseSystem(unique_id mod, const YAML::Node& node, data::data_manager*)
		{
			assert(false && "mods cannot create systems until scripting is introduced.");
		}

		void loadSystem(resource_base* r, data::data_manager* dataman)
		{
			//same as above
			return;
		}

		bool operator==(const curve_default_value &lhs, const curve_default_value &rhs)
		{
			return lhs.set == rhs.set
				&& lhs.value == rhs.value;
		}

		curve_type readCurveType(types::string s)
		{
			if (s == "const")
				return curve_type::const_c;
			else if (s == "step")
				return curve_type::step;
			else if (s == "linear")
				return curve_type::linear;
			else if (s == "pulse")
				return curve_type::pulse;
			else
				return curve_type::error;
		}

		VariableType readVariableType(types::string s)
		{
			if (s == "int" || s == "int32")
				return VariableType::INT;
			else if (s == "float")
				return VariableType::FLOAT;
			else if (s == "bool")
				return VariableType::BOOL;
			else if (s == "string")
				return VariableType::STRING;
			else if (s == "obj_ref")
				return VariableType::OBJECT_REF;
			//else if (s == "unique")
				//return VariableType::UNIQUE;
			else if (s == "int_vector")
				return VariableType::VECTOR_INT;
			else if (s == "float_vector")
				return VariableType::VECTOR_FLOAT;
			else if (s == "obj_ref_vector")
				return VariableType::VECTOR_OBJECT_REF;
			//else if (s == "unique_vector")
				//return VariableType::VECTOR_UNIQUE;
			else
				return VariableType::ERROR;
		}

		void parseCurve(unique_id mod, const YAML::Node& node, data::data_manager* dataman)
		{
			//curves:
			//		name:
			//			type: default: step //determines how the values are read between keyframes
			//			value: default: int32 //determines the value type
			//			sync: default: false //true if this should be syncronised to the client
			//			save: default false //true if this should be saved when creating a save file
			//	TODO:	trim: default false
			//	TODO:	default: value, [value1, value2, value3, ...] etc

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

		template<class T, class StringToT>
		std::vector<T> StringToVector(std::string_view str, StringToT converter)
		{
			// format accepted for curves

			//either : T
			// or : T, T, T, T, T
			// or : [T, T, T, T, T, T]

			//remove the braces at begining and end if present
			auto first_brace = str.find_first_of('[');
			if (first_brace != std::string_view::npos)
				str.remove_prefix(++first_brace);

			auto last_brace = str.find_last_of(']');
			if (last_brace != std::string_view::npos)
				str.remove_suffix(str.size() - last_brace);

			//split into csv
			std::vector<std::string_view> elements;
			split(str, ',', std::back_inserter(elements));

			if (std::any_of(std::begin(elements), std::end(elements), [](auto &&s) {
				return s.empty() //return true if the str is empty of consists of spaces
					|| std::all_of(std::begin(s), std::end(s), [](auto &&c) {return c == ' '; });
			}))
			{
				return std::vector<T>{};
			}

			//convert each one into T
			std::vector<T> out;
			for (const auto &e : elements)
				out.push_back(converter(to_string(e)));

			return out;
		}

		template<class T>
		std::vector<T> StringToVector(std::string_view str)
		{
			return StringToVector<T>(str, types::stov<T>);
		}

		types::string CurveValueToString(curve_default_value v)
		{
			if (!v.set)
				return types::string{};

			return std::visit([](auto &&v)->types::string {
				using type = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<type, curve_types::int_t>
					|| std::is_same_v<type, curve_types::float_t>
					|| std::is_same_v<type, curve_types::bool_t>
					|| std::is_same_v<type, curve_types::object_ref>
					|| std::is_same_v<type, curve_types::string>)
				{
					return to_string(v);
				}
				else if constexpr (std::is_same_v<type, curve_types::unique>)
				{
					return hades::data::GetAsString(v);
				}
				else if constexpr (std::is_same_v<type, curve_types::vector_int>
					|| std::is_same_v<type, curve_types::vector_float>
					|| std::is_same_v<type, curve_types::vector_object_ref>)
				{
					types::string out{ "[" };
					for (const auto &i : v)
					{
						out += to_string(i) + ", ";
					}

					//remove the final ", "
					out.pop_back();
					out.pop_back();
					out += "]";
					return out;
				}
				else if constexpr (std::is_same_v<type, curve_types::vector_unique>)
				{
					types::string out{ "[" };
					for (const auto &i : v)
					{
						out += hades::data::GetAsString(i) + ", ";
					}

					//remove the final ", "
					out.pop_back();
					out.pop_back();
					out += "]";
					return out;
				}
				else
				{
					static_assert(hades::always_false<type>::value, "unexpected type")
				}
			}, v.value);
		}

		curve_default_value StringToCurveValue(const curve* c, std::string_view str)
		{
			assert(c);
			const auto type = c->data_type;

			curve_default_value out;
			out.set = true;

			using namespace std::string_view_literals;
			static const auto true_str = { "true"sv, "TRUE"sv, "1"sv };

			if (type == VariableType::BOOL)
			{
				if (std::any_of(std::begin(true_str), std::end(true_str), [str](auto &&s) { return str == s; }))
					out.value = true;
				else
					out.value = false;
			}
			else if (type == VariableType::ERROR)
				assert(false);
			else if (type == VariableType::FLOAT)
			{
				out.value = std::stof(to_string(str));
			}
			else if (type == VariableType::INT || type == VariableType::OBJECT_REF)
			{
				out.value = std::stoi(to_string(str));
			}
			else if (type == VariableType::STRING)
				out.value = to_string(str);
			else if (type == VariableType::UNIQUE)
			{
				out.value = hades::data::GetUid(str);
			}
			else if (type == VariableType::VECTOR_FLOAT)
			{
				out.value = StringToVector<float>(str);
			}
			else if (type == VariableType::VECTOR_INT
				|| type == VariableType::VECTOR_OBJECT_REF)
			{
				out.value = StringToVector<int>(str);
			}
			else if (type == VariableType::VECTOR_UNIQUE)
			{
				out.value = StringToVector<unique_id>(str, [](auto &&str) {
					return hades::data::GetUid(str);
				});
			}
			else
				return curve_default_value{};

			return out;
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
				auto id = data->getUid(name);
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

				a->tex = data::FindOrCreate<resources::texture>(data::GetUid(texture_str), mod, data);
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
			catch (files::file_exception e)
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
