#include "Hades/simple_resources.hpp"

#include <array>

#include "yaml-cpp/yaml.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "Hades/Console.hpp"
#include "Hades/DataManager.hpp"

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
			for (auto p : pixels)
			{
				p = c.toInteger();
				if (counter++ == checker_scale)
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
			//textures: {
			//    default: {
			//        width: 0 #0 = autodetect, no size checking will be done
			//        height: 0 
			//        source: #empty source, no file is specified, will always be default error texture
			//        smooth: false
			//        repeating: false
			//        mips: false
			//    }
			//}

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
				auto id = dataman->getUid(n.as<types::string>());
				texture* tex;

				try
				{
					tex = dataman->get<texture>(id);
				}
				catch (data::resource_null r)
				{
					//resource doens't exist yet, create it
					auto texture_ptr = std::make_unique<texture>();
					dataman->set<texture>(id, std::move(texture_ptr));
					tex = &*texture_ptr;

					tex->height = d_height;
					tex->width = d_width;
					tex->smooth = d_smooth;
					tex->repeat = d_repeating;
					tex->mips = d_mips;
					tex->source = d_source;
				}
				catch (data::resource_wrong_type r)
				{
					//name is already used for something else, this cannnot be loaded
					auto name = n.as<types::string>();
					auto mod_ptr = dataman->getMod(mod);
					LOGERROR("Name collision with identifier: " + name + ", for texture while parsing mod: " + mod_ptr->name + ". Name has already been used for a different resource type.");
					//skip the rest of this loop and check the next node
					continue;
				}

				//only overwrite current values(or default if this is a new resource) if they are specified.
				auto width = n["width"];
				if (!width.IsNull())
					tex->width = width.as<texture::size_type>(d_width);

				auto height = n["height"];
				if (!height.IsNull())
					tex->height = height.as<texture::size_type>(d_height);

				
				auto smooth = n["smooth"];
				if (!smooth.IsNull())
					tex->smooth = smooth.as<bool>(d_smooth);

				auto repeat = n["repeating"];
				if (!repeat.IsNull())
					tex->repeat = repeat.as<bool>(d_repeating);

				auto mips = n["mips"];
				if (!mips.IsNull())
					tex->mips = mips.as<bool>(d_mips);

				auto source = n["source"];
				if (!source.IsNull())
					tex->source = source.as<types::string>(d_source);

				//if either size parameters are 0, then don't warn for size mismatch
				if (tex->width == 0 || tex->height == 0)
					tex->width = tex->height = 0;

				if (tex->width == 0)
					tex->value = generate_default_texture();
				else
					tex->value = generate_default_texture(tex->width, tex->height);
			}
		}

		void loadTexture(resources::resource_base*)
		{
			//if source is set
			//	try to load
			//else
			//	do nothing
		}
	}
}