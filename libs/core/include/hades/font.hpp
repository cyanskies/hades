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
		};

		namespace font_functions
		{
			font* find_or_create(data::data_manager&, unique_id, std::optional<unique_id> mod = {});
			// throws files::file_error if the file is missing or unreadable
			std::vector<std::byte> get_font_source_as_memory(const font&) noexcept;
		}
	}
}

#endif //!HADES_FONT_HPP
