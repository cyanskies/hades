#include "hades/texture.hpp"

#include <array>

#include "SFML/Graphics/Texture.hpp"

#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/sf_streams.hpp"

namespace hades
{
	void load_texture(resources::resource_type<sf::Texture>&, data::data_manager&);
}

namespace hades::resources
{
	struct texture : public resource_type<sf::Texture>
	{
		texture() : resource_type(load_texture) {}

		texture_size_t width = 0, height = 0;
		bool smooth = false, repeat = false, mips = false;
	};
}

namespace hades
{
	void parse_texture(unique_id mod, const data::parser_node& node, data::data_manager&);

	void register_texture_resource(data::data_manager &d)
	{
		using namespace std::string_view_literals;
		d.register_resource_type("textures"sv, parse_texture);
	}

	static sf::Texture generate_checkerboard_texture(texture_size_t width, texture_size_t height,
		texture_size_t checker_scale, colour c1, colour c2)
	{
		auto pixels = std::vector<sf::Uint8>{};
		const auto size = width * height;
		pixels.reserve(size);

		for (auto i = std::size_t{}; i < size; ++i)
		{
			auto coord = to_2d_index(i, integer_cast<std::size_t>(width));
			coord.first *= width / height;
			const auto cx = coord.first / checker_scale;
			const auto cy = coord.second / checker_scale;

			const auto is_colour = (cx + cy) % 2 == 0;
			const auto& c = is_colour ? c1 : c2;
			pixels.emplace_back(c.r);
			pixels.emplace_back(c.g);
			pixels.emplace_back(c.b);
			pixels.emplace_back(c.a);
		}

		sf::Texture t;
		t.create(width, height);
		t.update(pixels.data());
		return t;
	}

	//generates a group of
	static sf::Texture generate_default_texture(texture_size_t width = 32u, texture_size_t height = 32u)
	{
		static std::size_t counter = 0;
		constexpr auto size = std::size(colours::array);
		return generate_checkerboard_texture(width, height, std::min<texture_size_t>(width / 8, height / 8),
			colours::array[counter++ % size], colours::black);
	}

	void parse_texture(unique_id mod, const data::parser_node& node, data::data_manager &d)
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
		using namespace std::string_view_literals;
		constexpr auto resource_type = "textures"sv;

		const auto textures = node.get_children();

		for (const auto &t : textures)
		{
			const auto name = t->to_string();
			const auto id = d.get_uid(name);

			const auto tex = d.find_or_create<resources::texture>(id, mod);
			if (!tex)
				continue;

			using namespace data::parse_tools;
			tex->width	= get_scalar(*t, "width"sv,		tex->width);
			tex->height = get_scalar(*t, "height"sv,	tex->height);
			tex->smooth = get_scalar(*t, "smooth"sv,	tex->smooth);
			tex->repeat = get_scalar(*t, "repeating"sv, tex->repeat);
			tex->mips	= get_scalar(*t, "mips"sv,		tex->mips);
			tex->source = get_scalar(*t, "source"sv,	tex->source);

			//if either size parameters are 0, then don't warn for size mismatch
			if (tex->width == 0 || tex->height == 0)
				tex->width = tex->height = 0;
		}
	}

	void load_texture(resources::resource_type<sf::Texture> &r, data::data_manager &d)
	{
		using namespace std::string_literals;

		auto &tex = static_cast<resources::texture&>(r);

		if (!tex.source.empty())
		{
			//the mod not being available should be 'impossible'
			const auto mod = d.get<resources::mod>(tex.mod);

			try
			{
				auto fstream = sf_resource_stream{ mod->source, tex.source };

				//TODO: if !is_open generate err texture

				tex.value.loadFromStream(fstream);
			}
			catch (const files::file_error &e)
			{
				LOGERROR("Failed to load texture: "s + mod->source + "/"s + tex.source + ". "s + e.what());
				if (tex.width != 0 && tex.height != 0)
					tex.value = generate_default_texture(tex.width, tex.height);
				else
					tex.value = generate_default_texture();
			}

			tex.value.setSmooth(tex.smooth);
			tex.value.setRepeated(tex.repeat);

			if (tex.mips && !tex.value.generateMipmap())
				LOGWARNING("Failed to generate MipMap for texture: "s + d.get_as_string(tex.id));

			//if the width or height are 0, then don't warn about size mismatch
			//otherwise log unexpected size
			const auto size = tex.value.getSize();
			if (tex.width != 0 &&
				(size.x != tex.width || size.y != tex.height))
			{
				LOGWARNING("Loaded texture: "s + mod->source + "/"s + tex.source + ". Texture size different from requested. Requested("s +
					to_string(tex.width) + ", "s + to_string(tex.height) + "), Found("s + to_string(size.x) + ", "s + to_string(size.y) + ")"s);
				//NOTE: if the texture is the wrong size
				// then enable repeating, to avoid leaving
				// gaps in the world(between tiles and other such stuff).
				if(size.x < tex.width || size.y < tex.height)
					tex.value.setRepeated(true);
			}

			//if width or height are 0 then use them to store the textures size
			if (tex.width == 0 || tex.height == 0)
			{
				const auto tex_size = tex.value.getSize();
				tex.width = integer_cast<texture_size_t>(tex_size.x);
				tex.height = integer_cast<texture_size_t>(tex_size.y);
			}

			tex.loaded = true;
		}
	}

	namespace resources
	{
		namespace texture_functions
		{
			texture* find_create_texture(data::data_manager& d, const unique_id id , const unique_id mod)
			{
				return d.find_or_create<texture>(id, mod);
			}

			const texture* get_resource(const unique_id id)
			{
				return data::get<texture>(id);
			}

			const texture* get_resource(data::data_manager& d, unique_id i)
			{
				return d.get<texture>(i);
			}

			unique_id get_id(const texture* t) noexcept
			{
				assert(t);
				return t->id;
			}

			bool get_is_loaded(const texture* t) noexcept
			{
				assert(t);
				return t->loaded;
			}

			bool get_smooth(const texture* t) noexcept
			{
				assert(t);
				return t->smooth;
			}

			bool get_repeat(const texture* t) noexcept
			{
				assert(t);
				return t->repeat;
			}

			bool get_mips(const texture* t) noexcept
			{
				assert(t);
				return t->mips;
			}


			const sf::Texture& get_sf_texture(const texture* t) noexcept
			{
				assert(t);
				return t->value;
			}

			sf::Texture& get_sf_texture(texture* t) noexcept
			{
				assert(t);
				return t->value;
			}

			vector_t<texture_size_t> get_size(const texture* t) noexcept
			{
				assert(t);
				return { t->width, t->height };
			}

			void set_settings(texture* t, vector_t<texture_size_t> size, bool smooth, bool repeat, bool mips, bool loaded) noexcept
			{
				assert(t);
				t->width = size.x;
				t->height = size.y;
				t->smooth = smooth;
				t->repeat = repeat;
				t->mips = mips;

				t->value.setSmooth(smooth);
				t->value.setRepeated(repeat);
				if (mips)
					t->value.generateMipmap();

				t->loaded = loaded;
				return;
			}
		}
	
		texture_size_t get_max_texture_size()
		{
			return std::min(
				std::numeric_limits<texture_size_t>::max(),
				get_hardware_max_texture_size()
			);
		}

		texture_size_t get_hardware_max_texture_size()
		{
			return integer_cast<texture_size_t>(sf::Texture::getMaximumSize());
		}
	}
}