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
		unique_id default_font_id();

		struct font : public resource_type<sf::Font>
		{
			void load(data::data_manager&) final override;
			void serialise(const data::data_manager&, data::writer&) const final override;
			
			bool serialise_source() const final override
			{
				return true;
			}

			void serialise(std::ostream&) const final override;

			buffer source_buffer;
		};
	}
}

#endif //!HADES_FONT_HPP
