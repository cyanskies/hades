#ifndef HADES_TEXTURE_HPP
#define HADES_TEXTURE_HPP

#include "hades/data.hpp"
#include "hades/game_types.hpp"

namespace sf
{
	class Texture;
	class Image;
}

namespace hades
{
	void register_texture_resource(data::data_manager&);

	//TODO: create texture console variables(max_texture_size = auto)
	// get_max_texture_size // warnings are issued when going over this value
	// get_hardware_max_texture_size // errors are issued when going over this value

	namespace resources
	{
		struct texture;
		namespace texture_functions
		{
			texture* find_create_texture(data::data_manager&, unique_id, std::optional<unique_id> = {});
			const texture* get_resource(unique_id);
			texture* get_resource(data::data_manager&, unique_id, std::optional<unique_id> = {});
			resource_link<texture> make_resource_link(data::data_manager&, unique_id, unique_id from);
			// TODO: these should take const texture&
			unique_id get_id(const texture*) noexcept;
			bool get_is_loaded(const texture*) noexcept;
			bool get_smooth(const texture*) noexcept;
			bool get_repeat(const texture*) noexcept;
			bool get_mips(const texture*) noexcept;
			std::optional<colour> get_alpha_colour(const texture&) noexcept;
			// set_alpha_colour: reload the texture to see the effects of this.
			void set_alpha_colour(texture&, colour) noexcept;
			void clear_alpha(texture&) noexcept;
			// returns the current texture file source(path to archive/ path to file)
			std::tuple<const std::filesystem::path&, const std::filesystem::path&> get_loaded_paths(const texture&) noexcept;
			resource_base* get_resource_base(texture&) noexcept;
			const sf::Texture& get_sf_texture(const texture*) noexcept;
			sf::Texture& get_sf_texture(texture*) noexcept;
			vector2<texture_size_t> get_requested_size(const texture&) noexcept;
			vector2<texture_size_t> get_size(const texture&) noexcept;
			bool load_from_file(texture&, const std::filesystem::path&);
			void load_from_image(texture&, const sf::Image&);
			void set_settings(texture*, vector2<texture_size_t> size, bool smooth, bool repeat, bool mips, bool set_loaded) noexcept;
		}

		texture_size_t get_hardware_max_texture_size();
	}
}

#endif //!HADES_TEXTURE_HPP
