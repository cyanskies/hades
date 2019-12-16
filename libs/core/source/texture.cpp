#include "hades/texture.hpp"

#include <array>

#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/sf_streams.hpp"

namespace hades
{
	void parse_texture(unique_id mod, const data::parser_node& node, data::data_manager&);

	void register_texture_resource(data::data_manager &d)
	{
		using namespace std::string_view_literals;
		d.register_resource_type("textures"sv, parse_texture);
	}

	const auto colours = std::array{
		sf::Color::Magenta,
		sf::Color::White,
		sf::Color::Red,
		sf::Color::Green,
		sf::Color::Blue,
		sf::Color::Yellow,
		sf::Color::Cyan
	};

	sf::Texture generate_checkerboard_texture(resources::texture::size_type width, resources::texture::size_type height, resources::texture::size_type checker_scale,
		sf::Color c1, sf::Color c2)
	{
		std::vector<sf::Uint32> pixels(width * height);

		resources::texture::size_type counter = 0;
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
	sf::Texture generate_default_texture(resources::texture::size_type width = 32u, resources::texture::size_type height = 32u)
	{
		static std::size_t counter = 0;

		auto t = generate_checkerboard_texture(width, height, 16, colours[counter++ % colours.size()], sf::Color::Black);
		t.setRepeated(true);

		return t;
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

			//TODO: don't do this until someone tries to load the resource and fails
			if (tex->width == 0)
				tex->value = generate_default_texture();
			else
				tex->value = generate_default_texture(tex->width, tex->height);
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
				auto fstream = make_sf_stream( mod->source, tex.source );

				//TODO: if !is_open generate err texture

				tex.value.loadFromStream(fstream);
				tex.value.setSmooth(tex.smooth);
				tex.value.setRepeated(tex.repeat);

				if (tex.mips && !tex.value.generateMipmap())
					LOGWARNING("Failed to generate MipMap for texture: "s + d.get_as_string(tex.id));
			}
			catch (const files::file_error &e)
			{
				LOGERROR("Failed to load texture: "s + mod->source + "/"s + tex.source + ". "s + e.what());
			}

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
				tex.value.setRepeated(true);
			}

			//if width or height are 0 then use them to store the textures size
			if (tex.width == 0 || tex.height == 0)
			{
				const auto size = tex.value.getSize();
				tex.width = size.x;
				tex.height = size.y;
			}

			tex.loaded = true;
		}
	}

	namespace resources
	{
		texture::texture() : resource_type<sf::Texture>(load_texture) {}
		texture::size_type get_max_texture_size()
		{
			return std::min(
				std::numeric_limits<texture::size_type>::max(),
				get_hardware_max_texture_size()
			);
		}

		texture::size_type get_hardware_max_texture_size()
		{
			return sf::Texture::getMaximumSize();
		}
	}
}