#ifndef HADES_TEXTURE_HPP
#define HADES_TEXTURE_HPP

#include "SFML/Graphics/Texture.hpp"

#include "hades/data.hpp"
#include "hades/resource_base.hpp"

namespace hades
{
	void register_texture_resource(data::data_manager&);

	namespace resources
	{
		struct texture : public resource_type<sf::Texture>
		{
			texture();

			using size_type = types::uint16;
			//max texture size for older hardware is 512
			//max size for modern hardware is 8192 or higher
			size_type width = 0, height = 0;
			bool smooth = false, repeat = false, mips = false;
		};
	}
}

#endif !//HADES_TEXTURE_HPP
