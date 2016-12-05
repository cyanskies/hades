#include "Hades/simple_resources.hpp"

#include <array>

#include "yaml-cpp/yaml.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "Hades/DataManager.hpp"

namespace hades
{
	namespace resources
	{
		const size_t colour_count = 7;

		const std::array<sf::Color, colour_count> colours {
			sf::Color::White,
			sf::Color::Red,
			sf::Color::Green,
			sf::Color::Blue,
			sf::Color::Yellow,
			sf::Color::Magenta,
			sf::Color::Cyan
		};

		using texture_array = std::array<sf::Texture, colour_count>;
		
		//generates a group of 
		texture_array generate_default_textures()
		{
			texture_array out;
			for (auto c : colours)
			{
				std::size_t count = 0;
				sf::Image i;
				i.create(32, 32, sf::Color::Black);

				for (std::size_t r = 0; r < i.getSize().y; ++r)
				{
					for (std::size_t col = 0; col < i.getSize().x; ++col)
					{
						if ((r < 16 && col < 16) ||
							(r >= 16 && col >= 16) )
							i.setPixel(col, r, c);
					}
				}

				sf::Texture t;
				t.loadFromImage(i);
				out[count++] = t;
			}

			return out;
		}

		void parseTexture(data::UniqueId mod, YAML::Node& node, data::data_manager*)
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
			
			static const texture_array d_texture = generate_default_textures();
			static texture_array::size_type texture_count = 0;

			//for each node
			//{
			//	parse values
			//	load default error texture
			//	add to load queue
			//}

			for (auto n : node)
			{
				//collect data from yaml
				auto width = n["width"].as<texture::size_type>(d_width),
					height = n["height"].as<texture::size_type>(d_height);

				auto smooth = n["smooth"].as<bool>(d_smooth),
					repeat = n["repeating"].as<bool>(d_repeating),
					mips = n["mips"].as<bool>(d_mips);

				auto source = n["source"].as<types::string>(d_source);

				//load default texture in place

				//otherwise store the data
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