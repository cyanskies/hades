#ifndef HADES_TEXTURE_HPP
#define HADES_TEXTURE_HPP

#include "SFML/Graphics/Texture.hpp"

#include "hades/data.hpp"
#include "hades/game_types.hpp"
#include "hades/resource_base.hpp"

namespace hades
{
	void register_texture_resource(data::data_manager&);

	//TODO: create texture console variables(max_texture_size = auto)
	// get_max_texture_size // warnings are issued when going over this value
	// get_hardware_max_texture_size // errors are issued when going over this value

	namespace resources
	{
		struct texture : public resource_type<sf::Texture>
		{
			texture();

			texture_size_t width = 0, height = 0;
			bool smooth = false, repeat = false, mips = false;
		};

		texture_size_t get_max_texture_size();
		texture_size_t get_hardware_max_texture_size();
	}
}

#endif !//HADES_TEXTURE_HPP
