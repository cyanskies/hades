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

	static sf::Texture generate_checkerboard_texture(texture_size_t width, texture_size_t height, texture_size_t checker_scale,
		colour c1, colour c2)
	{
		//FIXME: textures generated here seem to have unintended transparency
		std::vector<uint32> pixels(width * height, c2.to_integer());

		for (auto i = std::size_t{}; i < size(pixels); ++i)
		{
			auto coord = to_2d_index(i, integer_cast<std::size_t>(width));
			coord.first *= width / height;
			const auto cx = coord.first / checker_scale;
			const auto cy = coord.second / checker_scale;

			if ((cx + cy) % 2 == 0)
				pixels[i] = c1.to_integer();
		}

		sf::Uint8* p = reinterpret_cast<sf::Uint8*>(pixels.data());
		sf::Texture t;
		t.create(width, height);
		t.update(p);
		return t;
	}

	//generates a group of
	static sf::Texture generate_default_texture(texture_size_t width = 32u, texture_size_t height = 32u)
	{
		static std::size_t counter = 0;
		constexpr auto size = std::size(colours::array);
		auto t = generate_checkerboard_texture(width, height, std::min(width / 8, height / 8), colours::array[counter++ % size], colours::black);
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
		texture::texture() : resource_type<sf::Texture>(load_texture) {}
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