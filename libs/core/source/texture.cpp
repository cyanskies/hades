#include "hades/texture.hpp"

#include <array>

#include "SFML/System/Err.hpp"
#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/sf_color.hpp"
#include "hades/sf_streams.hpp"
#include "hades/writer.hpp"

using namespace std::string_view_literals;

hades::unique_id error_texture_id = hades::unique_zero;

namespace hades::resources
{
	struct texture;
}

namespace hades
{
	static void load_texture(resources::texture&, data::data_manager&);
}

namespace hades::resources
{
	struct texture : public resource_type<sf::Texture>
	{
		void load(data::data_manager& d) final override
		{
			load_texture(*this, d);
			return;
		}  

		void serialise(const data::data_manager&, data::writer&) const final override;

		bool serialise_source() const noexcept final override
		{
			return source_mod == mod;
		}

		void serialise(std::ostream&) const final override;

		texture_size_t width = 0, height = 0;
		texture_size_t actual_width = 0, actual_height = 0;
		bool smooth = false, repeat = false, mips = false;
		// should_serialise_image_file: should be true if we assigned
		// an image or if we loaded the image from this mod
		bool should_serialise_image_file = false;
		std::optional<colour> alpha = {};
		//std::filesystem::path loaded_archive_path;
		//std::filesystem::path loaded_path;
		unique_id source_mod = {}; // the last mod that set the source or loaded* value
	};

	void texture::serialise(const data::data_manager& d, data::writer& w) const
	{
		//default texture
		const auto def = texture{};

		w.start_map(d.get_as_string(id));
		if(width != def.width)		w.write("width"sv, width);
		if(height != def.height)	w.write("height"sv, height);
		if(source != def.source)	w.write("source"sv, source.generic_string());
		if(smooth != def.smooth)	w.write("smooth"sv, smooth);
		if(repeat != def.repeat)	w.write("repeating"sv, repeat);
		if(mips != def.mips)		w.write("mips"sv, mips);
		if (alpha)
		{
			w.write("alpha-mask"sv);
			w.start_sequence();
			w.write(alpha->r);
			w.write(alpha->g);
			w.write(alpha->b);
			w.end_sequence();
		}
		w.end_map();
	}

	void texture::serialise(std::ostream& o) const
	{
		assert(o.good());
		if (!loaded_archive_path.empty())
		{
			// pull out of current archive
			auto strm = irfstream{ loaded_archive_path, loaded_path };
			if (!strm.is_open())
				log_error("Failed to write resource; unable to find file: " +
					loaded_archive_path.generic_string() + "/"  + loaded_path.generic_string());
			else if (!strm.good())
				log_error("Failed to write resource; unable to read resource: " +
					loaded_archive_path.generic_string() + "/" + loaded_path.generic_string());
			else
				o << strm.rdbuf();
		}
		else
		{
			auto strm = std::ifstream{ loaded_path, std::ios_base::binary };
			if(!strm.is_open())
				log_error("Failed to write resource; unable to find file: " + 
					(loaded_path.empty() ? source.generic_string() : loaded_path.generic_string()));
			else if (!strm.good())
				log_error("Failed to write resource; unable to read resource: " + loaded_path.generic_string());
			else
				o << strm.rdbuf();
		}

		return;
	}
}

namespace hades
{
	constexpr auto textures_str = "textures"sv;

	void parse_texture(unique_id mod, const data::parser_node& node, data::data_manager&);

	void register_texture_resource(data::data_manager &d)
	{
		using namespace std::string_view_literals;

		error_texture_id = d.get_uid("error-texture"sv);

		auto tex = d.find_or_create<resources::texture>(error_texture_id, {}, textures_str);
		tex->width = 32;
		tex->height = 32;
		tex->repeat = true;

		d.register_resource_type("textures"sv, parse_texture);
	}

	static sf::Texture generate_checkerboard_texture(texture_size_t width, texture_size_t height,
		texture_size_t checker_scale, colour c1, colour c2)
	{
		auto pixels = std::vector<sf::Uint8>{};
		const auto size = integer_cast<std::size_t>(width) * integer_cast<std::size_t>(height);
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
		auto sb = std::stringbuf{};
		const auto prev = sf::err().rdbuf(&sb);
		if (!t.create(width, height))
		{
			LOGERROR("Unable to create error texture");
			LOGERROR(sb.str());
		}
		else
			t.update(pixels.data());
        sf::err().rdbuf(prev);
		return t;
	}

	//generates a group of
	static sf::Texture generate_default_texture(texture_size_t width = 32u, texture_size_t height = 32u)
	{
		if (width == 0)
			width = 32u;
		if (height == 0)
			height = 32u;

		static std::size_t counter = 0;
		constexpr auto size = std::size(colours::all_colours);
		auto tex = generate_checkerboard_texture(width, height, std::min<texture_size_t>(width / 8, height / 8),
			colours::all_colours[counter++ % size], colours::black); // TODO: new colour array for bad colours
		tex.setRepeated(true);
		return tex;
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
		//		  alpha-mask: [r, g, b] or accepted colour name eg. black (see colour.hpp)

		using namespace std::string_view_literals;
        //constexpr auto resource_type = "textures"sv;

		const auto textures = node.get_children();

		for (const auto &t : textures)
		{
			const auto name = t->to_string();
			const auto id = d.get_uid(name);

			const auto tex = d.find_or_create<resources::texture>(id, mod, textures_str);
			if (!tex)
				continue;

			using namespace data::parse_tools;
			tex->width	= get_scalar(*t, "width"sv,		tex->width);
			tex->height = get_scalar(*t, "height"sv,	tex->height);
			tex->smooth = get_scalar(*t, "smooth"sv,	tex->smooth);
			tex->repeat = get_scalar(*t, "repeating"sv, tex->repeat);
			tex->mips	= get_scalar(*t, "mips"sv,		tex->mips);
			const std::filesystem::path old_source = tex->source;
			tex->source = get_scalar(*t, "source"sv,	tex->source.generic_string());
			if (tex->source != old_source)
				tex->source_mod = mod;


			// get alpha from rgb or by colour name
			const auto alpha = t->get_child("alpha-mask"sv);
			if (alpha)
			{
				if (alpha->is_sequence())
				{
					const auto rgb = alpha->to_sequence<uint8>();
					tex->alpha = colour{ rgb[0], rgb[1], rgb[2] };
				}
				else
				{
					const auto col_name = alpha->to_string();
					for (auto i = std::size_t{}; i < size(colours::all_colour_names); ++i)
					{
						if (colours::all_colour_names[i] == col_name)
						{
							assert(i < size(colours::all_colours));
							tex->alpha = colours::all_colours[i];
							break;
						}
					}
				}
			}

			//if either size parameters are 0, then don't warn for size mismatch
			if (tex->width == 0 || tex->height == 0)
				tex->width = tex->height = 0;
		}
	}

	static void load_texture(resources::texture &tex, data::data_manager &d)
	{
		using namespace std::string_literals;
		if (!tex.source.empty() && tex.source_mod)
		{
			//  the mod not being available should be 'impossible'
			const auto& mod = d.get_mod(tex.source_mod);

			try
			{
				auto fstream = sf_resource_stream{ mod.source, tex.source };
				auto sb = std::stringbuf{};
				const auto prev = sf::err().rdbuf(&sb);
				const auto f = make_finally([prev]() noexcept {
                    sf::err().rdbuf(prev);
					return;
					});
				if (!tex.value.loadFromStream(fstream))
					throw data::resource_error{ sb.str() };

				const auto& stream = fstream.stream();
				const auto mod_path = stream.mod_path();
				if (mod_path.has_extension())
				{
					tex.loaded_archive_path = mod_path;
					tex.loaded_path = stream.path();
				}
				else
				{
					tex.loaded_archive_path.clear();
					tex.loaded_path = mod_path / stream.path();
				}
			}
			catch (const files::file_error &e)
			{
				LOGERROR("Failed to load texture: "s + d.get_as_string(tex.id) + ". "s + e.what());
				tex.value = generate_default_texture(tex.width, tex.height);
				const auto tex_size = tex.value.getSize();
				tex.actual_width = integer_cast<texture_size_t>(tex_size.x);
				tex.actual_height = integer_cast<texture_size_t>(tex_size.y);
				tex.loaded = true;
				return;
			}

			if (tex.alpha)
			{
				auto img = tex.value.copyToImage();
				img.createMaskFromColor(to_sf_color(*tex.alpha));
				tex.value.update(img);
			}

			//if the width or height are 0, then don't warn about size mismatch
			//otherwise log unexpected size
			const auto size = tex.value.getSize();
			const auto too_small = size.x != tex.width || size.y != tex.height;
            if ((tex.width != 0 && too_small) ||
                (tex.height != 0 && too_small))
			{
				LOGWARNING("Loaded texture: "s + mod.source + "/"s + tex.source.generic_string() +
					". Texture size smaller than requested in mod: "s + mod.name + ", Requested("s +
					to_string(tex.width) + ", "s + to_string(tex.height) + "), Found("s 
					+ to_string(size.x) + ", "s + to_string(size.y) + ")"s);
				//NOTE: if the texture is the wrong size
				// then enable repeating, to avoid leaving
				// gaps in the world(between tiles and other such stuff).
				if(size.x < tex.width || size.y < tex.height)
					tex.value.setRepeated(true);
			}

			const auto tex_size = tex.value.getSize();
			tex.actual_width = integer_cast<texture_size_t>(tex_size.x);
			tex.actual_height = integer_cast<texture_size_t>(tex_size.y);
		}
		else
		{
			tex.value = generate_default_texture(tex.width, tex.height);
		}

		tex.value.setSmooth(tex.smooth);
		if(!tex.value.isRepeated()) // don't disable repeat if it has been set(probably from generate_default_tex)
			tex.value.setRepeated(tex.repeat);

		if (tex.mips && !tex.value.generateMipmap())
			LOGWARNING("Failed to generate MipMap for texture: "s + d.get_as_string(tex.id));

		tex.loaded = true;
		return;
	}

	namespace resources
	{
		namespace texture_functions
		{
			texture* find_create_texture(data::data_manager& d, const unique_id id , const std::optional<unique_id> mod)
			{
				return d.find_or_create<texture>(id, mod, textures_str);
			}

			const texture* get_resource(const unique_id id)
			{
				try
				{
					return data::get<texture>(id);
				}
				catch (data::resource_null& e)
				{
					using namespace std::string_literals;
					LOGERROR("Unable to get texture. "s + e.what());
					return data::get<texture>(error_texture_id);
				}
			}

			texture* get_resource(data::data_manager& d, const unique_id i, const std::optional<unique_id> mod)
			{
				return d.get<texture>(i, mod);
			}

			resource_link<texture> make_resource_link(data::data_manager& d, const unique_id id, const unique_id from)
			{
				return d.make_resource_link<texture>(id, from, [](data::data_manager& d, const unique_id target)->const texture* {
						return d.get<texture>(target, data::no_load);
					});
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

			std::optional<colour> get_alpha_colour(const texture& t) noexcept
			{
				return t.alpha;
			}

			void set_alpha_colour(texture& t, colour c) noexcept
			{
				t.alpha = c;
				t.loaded = false;
				return;
			}

			void clear_alpha(texture& t) noexcept
			{
				t.alpha.reset();
				t.loaded = false;
				return;
			}

			std::tuple<const std::filesystem::path&, const std::filesystem::path&> get_loaded_paths(const texture& t) noexcept
			{
				return std::tie(t.loaded_archive_path, t.loaded_path);
			}

			resource_base* get_resource_base(texture& t) noexcept
			{
				return &t;
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

			vector_t<texture_size_t> get_requested_size(const texture& t) noexcept
			{
				return { t.width, t.height };
			}

			vector_t<texture_size_t> get_size(const texture& t) noexcept
			{
				const auto width = t.width == 0 ? t.actual_width : t.width;
				const auto height = t.height == 0 ? t.actual_height : t.height;
				return { width, height };
			}

			bool load_from_file(texture& t, const std::filesystem::path& p)
			{
				auto strm = sf_stream_wrapper<std::ifstream>{ p, std::ios::binary };
				if (t.value.loadFromStream(strm))
				{
					t.loaded_archive_path.clear();
					t.loaded_path = p;
					const auto [x, y] = t.value.getSize();
					t.actual_width = integer_cast<texture_size_t>(x);
					t.actual_height = integer_cast<texture_size_t>(y);
					t.source_mod = t.mod;
					set_settings(&t,
						{ integer_cast<texture_size_t>(x), integer_cast<texture_size_t>(y) },
						t.smooth, t.repeat, t.mips, true);
					return true;
				}

				return false;
			}

			void load_from_image(texture& t, const sf::Image& i)
			{
				const auto [x, y] = i.getSize();
				if (t.value.loadFromImage(i))
				{
					t.actual_width = integer_cast<texture_size_t>(x);
					t.actual_height = integer_cast<texture_size_t>(y);
					set_settings(&t,
						{ integer_cast<texture_size_t>(x), integer_cast<texture_size_t>(y) },
						t.smooth, t.repeat, t.mips, true);
				}
				else
					throw data::resource_error{ "Failed to load texture from image" };
				return;
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
				{
					auto sb = std::stringbuf{};
					const auto prev = sf::err().rdbuf(&sb);
					if (!t->value.generateMipmap())
					{
						LOGERROR("Unable to generate mipmap for texture");
						LOGERROR(sb.str());
					}
                    sf::err().rdbuf(prev);
				}

				t->loaded = loaded;
				return;
			}
		}

		texture_size_t get_hardware_max_texture_size()
		{
			return integer_clamp_cast<texture_size_t>(sf::Texture::getMaximumSize());
		}
	}
}
