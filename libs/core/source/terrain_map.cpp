#include "hades/terrain_map.hpp"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/OpenGL.hpp"

#include "hades/rectangle_math.hpp"
#include "hades/shader.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

const auto vertex_source = R"(
void main()
{{
    // transform the vertex position
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	// add pseudo height
	{}
    // transform the texture coordinates
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    // forward the vertex color
    gl_FrontColor = gl_Color;
}})";

constexpr auto vertex_add_height = "gl_Position.y += gl_Color.g;";
constexpr auto vertex_no_height = "";

//https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
constexpr auto fragment_source = R"(uniform sampler2D tex;
void main()
{{
	gl_FragColor = texture2D(tex, gl_TexCoord[0].xy);
	gl_FragDepth = {} (gl_Color.r / 255.0f);
}})";

constexpr auto fragment_add_height_depth = "((255.f - gl_Color.g) / 255.f) +";
constexpr auto fragment_no_height_depth = "";

auto terrain_map_shader_id = hades::unique_zero;
auto terrain_map_shader_no_height_id = hades::unique_zero;

namespace hades
{
	namespace shdr_funcs = resources::shader_functions;

	void register_terrain_map_resources(data::data_manager& d)
	{
		register_texture_resource(d);
		register_terrain_resources(d, resources::texture_functions::make_resource_link);

		{ // make normal terrain shader
			terrain_map_shader_id = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_id);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(fragment_source, fragment_add_height_depth));
			shdr_funcs::set_vertex(*shader, std::format(vertex_source, vertex_add_height));

			auto uniforms = resources::shader_uniform_map{
				{
					"tex"s,
					resources::uniform{
						resources::uniform_type_list::texture,
						sf::Shader::CurrentTexture
					}
				}
			};

			shdr_funcs::set_uniforms(*shader, std::move(uniforms));
		}

		{ // make flat terrain shader
			terrain_map_shader_no_height_id = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_no_height_id);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(fragment_source, fragment_no_height_depth));
			shdr_funcs::set_vertex(*shader, std::format(vertex_source, vertex_no_height));

			auto uniforms = resources::shader_uniform_map{
				{
					"tex"s,
					resources::uniform{
						resources::uniform_type_list::texture,
						sf::Shader::CurrentTexture
					}
				}
			};

			shdr_funcs::set_uniforms(*shader, std::move(uniforms));
		}
	}

	immutable_terrain_map::immutable_terrain_map(const terrain_map& t) :
		_tile_layer{ t.tile_layer }
	{
		for (const auto& l : t.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void immutable_terrain_map::create(const terrain_map& t)
	{
		_tile_layer.create(t.tile_layer);
		_terrain_layers.clear();
		for (const auto& l : t.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void immutable_terrain_map::draw(sf::RenderTarget& t, const sf::RenderStates& s) const
	{
		for (auto iter = std::rbegin(_terrain_layers);
			iter != std::rend(_terrain_layers); ++iter)
			t.draw(*iter, s);

		t.draw(_tile_layer, s);
	}

	rect_float immutable_terrain_map::get_local_bounds() const noexcept
	{
		return _tile_layer.get_local_bounds();
	}

	//==================================//
	//		mutable_terrain_map			//
	//==================================//

	mutable_terrain_map::mutable_terrain_map() : 
		_shader{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_id)) },
		_shader_no_height{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_no_height_id)) }
	{}

	mutable_terrain_map::mutable_terrain_map(terrain_map t) : mutable_terrain_map{}
	{
		reset(std::move(t));
	}

	void mutable_terrain_map::reset(terrain_map t)
	{
		const auto& settings = *resources::get_terrain_settings();
		const auto tile_size = settings.tile_size;
		const auto width = t.tile_layer.width;
		const auto& tiles = t.tile_layer.tiles;

		_local_bounds = rect_float{
			0.f, 0.f,
			float_cast(width * tile_size),
			float_cast((tiles.size() / width) * tile_size)
		};

		_map = std::move(t);
		_needs_apply = true;
		return;
	}

	class terrain_map_texture_limit_exceeded : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	static void generate_layer(const std::size_t layer_index,
		mutable_terrain_map::map_info& info, const tile_map& map,
		const std::vector<std::uint8_t>& heightmap, const terrain_index_t world_width)
	{
		const auto index = integer_cast<std::uint8_t>(layer_index);

		const auto s = size(map.tiles);
		for (auto i = std::size_t{}; i < s; ++i)
		{
			const auto& t = map.tiles[i];
			const auto tile = get_tile(map, t);
			if (!tile.tex)
				continue;

			// get a index for the texture
			auto tex_index = std::numeric_limits<std::size_t>::max();
			const auto tex_size = size(info.texture_table);
			for (auto j = std::size_t{}; j < tex_size; ++j)
			{
				if (info.texture_table[j] == tile.tex)
				{
					tex_index = j;
					break;
				}
			}

			if (tex_index > tex_size)
			{
				info.texture_table.emplace_back(tile.tex);
				tex_index = tex_size;
			}

			const auto pos = from_tile_index(i, world_width - 1);
			const auto indicies = to_terrain_index(pos, world_width);

			constexpr auto top_left = enum_type(rect_corners::top_left),
				top_right = enum_type(rect_corners::top_right),
				bottom_right = enum_type(rect_corners::bottom_right),
				bottom_left = enum_type(rect_corners::bottom_left);

			std::array<std::uint8_t, 4> corner_height;
			corner_height[top_left]		= heightmap[indicies[top_left]];
			corner_height[top_right]	= heightmap[indicies[top_right]];
			corner_height[bottom_right] = heightmap[indicies[bottom_right]];
			corner_height[bottom_left]	= heightmap[indicies[bottom_left]];

			// add tile to the tile list
			info.tile_info.emplace_back(integer_cast<tile_index_t>(i), corner_height, tile.left, tile.top, integer_cast<std::uint8_t>(tex_index), index);
		}

		return;
	}

	void mutable_terrain_map::apply()
	{
		if (!_needs_apply)
			return;

		// regenerate everything else
		_info.tile_info.clear();
		_info.texture_table.clear();

		try
		{
			const auto width = get_width(_map);

			auto i = std::size_t{};
			const auto s = size(_map.terrain_layers);
			for (; i < s; ++i)
				generate_layer(i, _info, _map.terrain_layers[i], _map.heightmap, width);

			generate_layer({}, _info, _map.tile_layer, _map.heightmap, width);
		}
		catch (const overflow_error&)
		{
			log_warning("terrain map has too many terrain layers (> 255)"sv);
		}
		catch (const terrain_map_texture_limit_exceeded& e)
		{
			log_warning(e.what());
		}

		std::ranges::sort(_info.tile_info, {}, &map_tile::texture);

		_quads.clear();
		_quads.reserve(size(_info.tile_info));

		const auto& settings = *resources::get_terrain_settings();
		const auto tile_size = settings.tile_size;
		const auto tile_sizef = float_cast(tile_size);
		const auto width = _map.tile_layer.width;

		for (const auto& tile : _info.tile_info)
		{
			const auto [x, y] = to_2d_index(tile.index, width);
			const auto pos = vector2_float{
				float_cast(x) * tile_sizef,
				float_cast(y) * tile_sizef
			};
			const auto tex_coords = rect_float{
				float_cast(tile.left),
				float_cast(tile.top),
				tile_sizef,
				tile_sizef
			};

			std::array<colour, 4> colours;
			colours.fill(colour{ tile.layer, 255, 255 });

			for (auto i = std::size_t{}; i < size(tile.height); ++i)
				colours[i].g = tile.height[i];

			const auto quad = make_quad_animation(pos, { tile_sizef, tile_sizef }, tex_coords, colours);
			_quads.append(quad);
		}

		_quads.apply();

		_needs_apply = false;
		return;
	}

	void mutable_terrain_map::draw(sf::RenderTarget& t, const sf::RenderStates& states) const
	{
		assert(!_needs_apply);
		auto s = states;
		s.shader = _show_height ? _shader.get_shader() : _shader_no_height.get_shader();
		const auto beg = begin(_info.tile_info);
		const auto ed = end(_info.tile_info);
		auto it = beg;

#define DEPTH 1 // enable depth testing
#if DEPTH
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_ALPHA_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LEQUAL);
		glDepthRange(1.0f, 0.0f);
		glAlphaFunc(GL_GREATER, 0.0f);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
#endif

		while (it != ed)
		{
			auto tex = it->texture;
			auto index = distance(beg, it);
			while (it != ed && it->texture == tex)
				++it;

			auto last = distance(beg, it);
			s.texture = &resources::texture_functions::get_sf_texture(_info.texture_table[tex].get());
			_quads.draw(t, index, last - index, s);
		}

#if DEPTH
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);
		glDepthMask(GL_FALSE);
#endif

		return;
	}

	void mutable_terrain_map::place_tile(const tile_position p, const resources::tile& t)
	{
		hades::place_tile(_map, p, t);
		_needs_apply = true;
		return;
	}

	void mutable_terrain_map::place_terrain(const terrain_vertex_position p, const resources::terrain* t)
	{
		hades::place_terrain(_map, p, t);
		_needs_apply = true;
		return;
	}
}
