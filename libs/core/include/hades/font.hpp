#ifndef HADES_FONT_HPP
#define HADES_FONT_HPP

#include "SFML/Graphics/Font.hpp"

#include "hades/data.hpp"
#include "hades/files.hpp"
#include "hades/resource_base.hpp"

namespace hades
{
	void register_font_resource(data::data_manager&);

	namespace resources
	{
		struct font : public resource_type<sf::Font>
		{
			font();

			buffer source_buffer;
		};
	}
}

#endif //!HADES_FONT_HPP
