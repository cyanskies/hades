#include "hades/terrain_map.hpp"

#include <numbers>

#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/OpenGL.hpp"

#include "hades/draw_tools.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/shader.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

const auto vertex_source = R"(
#version 120
{}
uniform mat4 rotation_matrix;
uniform float height_multiplier;
varying float model_view_vertex_y;

void main()
{{
	// pass position along for shadowmapping
	{}
	// store world position of vertex for fragment shader
	model_view_vertex_y = (rotation_matrix * gl_Vertex).y;
	// copy input so we can modify it
	vec4 vert = gl_ModelViewMatrix * gl_Vertex;
	// move vertex up to simulate height
    vert.y -= (float(gl_Color.r) * 255) * height_multiplier;
    // transform the vertex position
	gl_Position = gl_ProjectionMatrix * vert;
	// transform the texture coordinates
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    // forward the vertex color
    gl_FrontColor = gl_Color;
}})";

constexpr auto vertex_shadow_uniforms = "uniform vec2 world_size;uniform float tile_size;varying vec2 position;";
constexpr auto vertex_no_shadow_uniforms = "";
constexpr auto vertex_shadow_position = "position = (gl_Vertex.xy / tile_size) / world_size;";
constexpr auto vertex_no_shadow_position = "// NOTE: not calculating shadowmapping position";

//https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
constexpr auto fragment_source = R"(
#version 120
uniform sampler2D tex;
uniform float y_offset;
uniform float y_range;
varying float model_view_vertex_y;

void main()
{{
	// calculate fragment depth along y axis
	gl_FragDepth = 1.f - ((model_view_vertex_y - y_offset) / y_range);
	// frag colour
	{}
}})";

constexpr auto fragment_normal_color = "gl_FragColor = texture2D(tex, gl_TexCoord[0].xy);";
constexpr auto fragment_depth_color = "gl_FragColor = vec4(gl_FragDepth, gl_FragDepth, gl_FragDepth, 1.f);tex;";

constexpr auto shadow_fragment_source = R"(
#version 120
//uniform sampler2D sunlight_map;
uniform float y_offset;
uniform float y_range;
varying float model_view_vertex_y;

const vec4 sun_colour = vec4(1.f, 1.f, .6f, .2f);
const vec4 shadow_col = vec4(0, 0, 0, 0.3f);

void main()
{
	// calculate fragment depth along y axis
	gl_FragDepth = 1.f - ((model_view_vertex_y - y_offset) / y_range);
	// shadow info
	float shadow_height = gl_Color.g;
	float frag_height = gl_Color.r;
	//vec4 light_col = vec4(sun_colour.rgb, mix(0.f, sun_colour.a, shadow_height));
	// frag colour
	gl_FragColor = mix(vec4(0.f, 0.f, 0.f, 0.f) , shadow_col, float(frag_height < shadow_height) * 1.f);
})";

static auto terrain_map_shader_id = hades::unique_zero;
static auto terrain_map_shader_debug_id = hades::unique_zero;
static auto terrain_map_shader_shadows_lighting = hades::unique_zero;

namespace hades
{
	namespace shdr_funcs = resources::shader_functions;

	void register_terrain_map_resources(data::data_manager& d)
	{
		register_texture_resource(d);
		register_terrain_resources(d, resources::texture_functions::make_resource_link);
		
		auto uniforms = resources::shader_uniform_map{
				{
					"tex"s, // current texture
					resources::uniform{
						resources::uniform_type_list::texture,
						sf::Shader::CurrentTexture
					}
				},{
					"rotation_matrix"s,
					resources::uniform{
						resources::uniform_type_list::matrix4,
						sf::Glsl::Mat4{ sf::Transform::Identity } // 0.f
					}
				},{
					"height_multiplier"s,
					resources::uniform{
						resources::uniform_type_list::float_t,
						{} // 0.f
					}
				},{
					"y_offset"s,
					resources::uniform{
						resources::uniform_type_list::float_t,
						{} // 0.f
					}
				},{
					"y_range"s,
					resources::uniform{
						resources::uniform_type_list::float_t,
						{} // 0.f
					}
				}
		};

		if(terrain_map_shader_id == unique_zero)
		{ // make heightmap terrain shader
			terrain_map_shader_id = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_id);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(fragment_source, fragment_normal_color));
			shdr_funcs::set_vertex(*shader, std::format(vertex_source, vertex_no_shadow_uniforms, vertex_no_shadow_position));
			shdr_funcs::set_uniforms(*shader, uniforms); // copy uniforms(we move it later)
		}

		if (terrain_map_shader_debug_id == unique_zero)
		{ // make heightmap debug terrain shader
			terrain_map_shader_debug_id = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_debug_id);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(fragment_source, fragment_depth_color));
			shdr_funcs::set_vertex(*shader, std::format(vertex_source, vertex_no_shadow_uniforms, vertex_no_shadow_position));
			shdr_funcs::set_uniforms(*shader, std::move(uniforms)); // copy uniforms(we move it later)
		}

		uniforms.clear();
		uniforms = resources::shader_uniform_map{
				{
					"rotation_matrix"s,
					resources::uniform{
						resources::uniform_type_list::matrix4,
						sf::Glsl::Mat4{ sf::Transform::Identity } // 0.f
					}
				},{
					"height_multiplier"s,
					resources::uniform{
						resources::uniform_type_list::float_t,
						{} // 0.f
					}
				},{
					"y_offset"s,
					resources::uniform{
						resources::uniform_type_list::float_t,
						{} // 0.f
					}
				},{
					"y_range"s,
					resources::uniform{
						resources::uniform_type_list::float_t,
						{} // 0.f
					}
				}//,{
				//	"tile_size"s,
				//	resources::uniform{
				//		resources::uniform_type_list::float_t,
				//		{} // 0.f
				//	}
				//},{
				//	"world_size"s,
				//	resources::uniform{
				//		resources::uniform_type_list::vector2f,
				//		{} // (0, 0)
				//	}
				//},{
				//	"sunlight_map"s,
				//	resources::uniform{
				//		resources::uniform_type_list::texture
				//	}
				//}
		};

		// terrain shadow rendering shader
		if (terrain_map_shader_shadows_lighting == unique_zero)
		{
			terrain_map_shader_shadows_lighting = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_shadows_lighting);
			assert(shader);

			shdr_funcs::set_fragment(*shader, shadow_fragment_source);
			shdr_funcs::set_vertex(*shader, std::format(vertex_source, vertex_shadow_uniforms, vertex_shadow_position));
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
		_settings{ resources::get_terrain_settings() },
		_shader{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_id)) }, 
		_shader_debug_depth{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_debug_id)) },
		_shader_shadows_lighting{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_shadows_lighting)) },
		_sunlight_table{ std::make_unique<sf::Texture>() }
	{}

	mutable_terrain_map::mutable_terrain_map(terrain_map t) : mutable_terrain_map{}
	{
		reset(std::move(t));
	}

	void mutable_terrain_map::reset(terrain_map t)
	{
		const auto& tile_size = _settings->tile_size;
		const auto& width = t.tile_layer.width;
		const auto& tiles = t.tile_layer.tiles;

		_local_bounds = rect_float{
			0.f, 0.f,
			float_cast(width * tile_size),
			float_cast((tiles.size() / width) * tile_size)
		};

		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
			shdr->set_uniform("height_multiplier"sv, _show_height * 1.f);

		const auto terrain_size = static_cast<vector2<unsigned int>>(get_vertex_size(t));
		if (!_sunlight_table->create({ terrain_size.x, terrain_size.y }))
		{
			throw runtime_error{
				std::format("Unable to create texture for terrain shadowmap. Size was: ({}, {})"sv,
				terrain_size.x, terrain_size.y)
			};
		}

		_sunlight_table->setSmooth(true);

		_shader_shadows_lighting.set_uniform("tile_size"sv, float_cast(tile_size));
		_shader_shadows_lighting.set_uniform("world_size"sv, static_cast<vector2_float>(get_vertex_size(t)));

		_map = std::move(t);

		set_world_rotation(0.f);

		_needs_apply = true;
		return;
	}

	void mutable_terrain_map::set_height_enabled(bool b) noexcept
	{
		_show_height = b;
		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
			shdr->set_uniform("height_multiplier"sv, _show_height * 1.f);
		return;
	}

	static void generate_layer(const std::size_t layer_index,
		mutable_terrain_map::map_info& info, const tile_map& map,
		const terrain_map& world, const terrain_index_t world_width)
	{
		const auto index = integer_cast<std::uint8_t>(layer_index);

		const auto s = size(map.tiles);
		for (auto i = tile_index_t{}; i < s; ++i)
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

			const auto corner_height = get_height_at(pos, world);
			// add tile to the tile list
			info.tile_info.emplace_back(integer_cast<tile_index_t>(i), corner_height, tile.left, tile.top, integer_cast<std::uint8_t>(tex_index), index);
		}

		return;
	}

	// layer constants
	constexpr auto grid_layer = 0;
	constexpr auto cliff_layer = 2; // also the world edge skirt if we add one(might just add enough of a margin to hide the world edge)
	constexpr auto tile_layer = 1;
	constexpr auto protected_layers = std::max({ grid_layer, tile_layer, cliff_layer });

	static const resources::texture* generate_grid(const terrain_map& map,
		const resources::terrain_settings &settings, const float tile_sizef,
		quad_buffer& quads)
	{
		const auto s = size(map.tile_layer.tiles);
		assert(settings.grid_terrain);
		const auto& grid_tiles = resources::get_transitions(*settings.grid_terrain, resources::transition_tile_type::all, settings);
		assert(!empty(grid_tiles));
		const auto grid_tile = grid_tiles.front();
		const auto width = get_width(map);
		for (auto i = tile_index_t{}; i < s; ++i)
		{
			const auto pos = from_tile_index(i, width - 1);
			const auto corner_height = get_height_at(pos, map);

			const auto world_pos = vector2_float{
				float_cast(pos.x) * tile_sizef,
				float_cast(pos.y) * tile_sizef
			};
			const auto tex_coords = rect_float{
				float_cast(grid_tile.left),
				float_cast(grid_tile.top),
				tile_sizef,
				tile_sizef
			};

			std::array<colour, 4> colours; // uninitialised
			colours.fill(colour{});

			for (auto j = std::size_t{}; j < size(corner_height); ++j)
				colours[j].r = corner_height[j];

			const auto quad = make_quad_animation(world_pos, { tile_sizef, tile_sizef }, tex_coords, colours);
			quads.append(quad);
		}
		return grid_tile.tex.get();
	}

	static void generate_shadow_row(const std::uint8_t sun_rise, const terrain_map& map,
		const terrain_index_t row_begin, const terrain_index_t row_end, sf::Image& img, const unsigned int img_y)
	{
		assert(img.getSize().y >= img_y &&
			img.getSize().x >= integer_cast<unsigned int>(row_end - row_begin));
		const auto get_max_height = [&map](terrain_index_t i) noexcept {
			const auto [h1, h2, h3, h4] = get_height_at(i, map);
			return std::max({ h1, h2, h3, h4 });
			};

		auto x = unsigned int{};
		auto h = get_max_height(row_begin);

		img.setPixel({ x++, img_y }, sf::Color{ h, 0, 0, 255 });

		while (std::cmp_less(row_begin + x, row_end))
		{
			if (h < sun_rise)
				h = {};
			else
				h -= sun_rise;
			
			const auto h2 = get_max_height(row_begin + x);
			if (h2 > h)
				h = h2;
			
			img.setPixel({ x, img_y }, sf::Color{ h, 0, 0, 255 });
			++x;
		}
		return;
	}

	static sf::Image generate_shadowmap(const float sun_angle_radian, const float tile_size,
		const terrain_map& map)
	{
		// TODO: clean up type usage here, heightmap can be indexed with size_t
		//		but img can only be indexed with unsigned
		const auto vert_size = get_vertex_size(map);

		auto img = sf::Image{};
		img.create(sf::Vector2u{ integer_cast<unsigned>(vert_size.x), integer_cast<unsigned>(vert_size.y) }, sf::Color::Black);
		
		// find the y distance between tile_size x
		const auto sun_unit_vect = to_vector(pol_vector2_t<float>{ sun_angle_radian + std::numbers::pi_v<float>, 1.f });
		const auto sun_step_mag = std::sqrt(1 + std::pow(sun_unit_vect.x / sun_unit_vect.y, 2.f));
		const auto sun_vector = sun_unit_vect * sun_step_mag * tile_size;
		const auto sun_rise = integral_cast<std::uint8_t>(std::abs(sun_vector.y));

		for (auto i = terrain_index_t{}; i < vert_size.y; ++i)
		{
			const auto row_begin = vert_size.x * i;
			generate_shadow_row(sun_rise, map, row_begin, row_begin + vert_size.x, img, integer_cast<unsigned int>(i));
		}

		return img;
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
			// NOTE: layer 0 is reserved for the grid
			//		layer 2 is reserved for tile_layer
			//		layer 1 is reserved for cliffs
			for (; i < s; ++i)
				generate_layer(i + protected_layers, _info, _map.terrain_layers[i], _map, width);

			generate_layer(tile_layer, _info, _map.tile_layer, _map, width);
		}
		catch (const overflow_error&)
		{
			// TODO: this doesnt seem to be a problem anymore
			log_warning("terrain map has too many terrain layers (> 255)"sv);
		}

		std::ranges::sort(_info.tile_info, {}, &map_tile::texture);

		std::ranges::stable_sort(_info.tile_info, std::greater{}, &map_tile::layer);

		_quads.clear();
		_quads.reserve(size(_info.tile_info));

		assert(_settings);
		const auto tile_size = _settings->tile_size;
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
			colours.fill(colour{});

			for (auto i = std::size_t{}; i < size(tile.height); ++i)
				colours[i].r = tile.height[i];

			const auto quad = make_quad_animation(pos, { tile_sizef, tile_sizef }, tex_coords, colours);
			_quads.append(quad);
		}

		// cliffs begin
		//Cliffs
		// add to quadmap and store start index of cliffs

		// shadows
		// TODO: set shadow strength based on the angle of the sun
		_shadow_start = std::size(_quads);
		const auto shadow_map = generate_shadowmap(_sun_angle, tile_sizef, _map);
		assert(shadow_map.getSize() == _sunlight_table->getSize());
		_sunlight_table->update(shadow_map);

		_shadow_tile_info.clear();
		std::ranges::transform(_info.tile_info, std::inserter(_shadow_tile_info, end(_shadow_tile_info)),
			[](const map_tile& m) noexcept {
				return shadow_tile{ m.index, m.height };
			});

		const auto sun_vect = to_vector(basic_pol_vector<float, 3>{ 0.f, _sun_angle, 1.f });

		for (const auto& tile : _shadow_tile_info)
		{
			const auto [x, y] = to_2d_index(tile.index, width);
			const auto pos = vector2_float{
				float_cast(x) * tile_sizef,
				float_cast(y) * tile_sizef
			};
			
			constexpr auto top_left = enum_type(rect_corners::top_left),
				top_right = enum_type(rect_corners::top_right),
				//bottom_right = enum_type(rect_corners::bottom_right),
				bottom_left = enum_type(rect_corners::bottom_left);

			// calculate normal
			// we only need to calculate normal for one of the triangles
			// TODO: calculate normal for each  vertex, by adverageing the normals of adjacent tris
			const auto p1 = vector3<float>{ pos.x, pos.y, float_cast(tile.height[top_left]) };
			const auto p2 = vector3<float>{ pos.x + tile_sizef, pos.y, float_cast(tile.height[top_right]) };
			const auto p3 = vector3<float>{ pos.x, pos.y + tile_sizef, float_cast(tile.height[bottom_left]) };

			const auto u = p2 - p1;
			const auto v = p3 - p1;

			const auto normal = vector::unit(vector::cross(u, v));
			const auto dot = vector::dot(normal, sun_vect);
			const auto light_val = dot < 0.f ? integral_clamp_cast<std::uint8_t>(dot * 255.f * -1.f) : std::uint8_t{};
			
			std::array<colour, 4> colours;
			colours.fill(colour{ {}, light_val });

			for (auto i = std::size_t{}; i < size(tile.height); ++i)
				colours[i].r = tile.height[i];

			const auto quad = make_quad_colour({ pos, { tile_sizef, tile_sizef } }, colours);
			_quads.append(quad);
		}

		_shader_shadows_lighting.set_uniform("sunlight_map"sv, &*_sunlight_table);

		// grid
		_grid_start = std::size(_quads);
		_grid_tex = generate_grid(_map, *_settings, tile_sizef, _quads);

		_quads.apply();

		_needs_apply = false;
		return;
	}

	void mutable_terrain_map::draw(sf::RenderTarget& t, const sf::RenderStates& states) const
	{
		assert(!_needs_apply);
		
		auto s = states;
		s.shader = _debug_depth ? _shader_debug_depth.get_shader() : _shader.get_shader();
		const auto beg = begin(_info.tile_info);
		const auto ed = end(_info.tile_info);
		auto it = beg;

		depth_buffer::setup();
		depth_buffer::clear();
		depth_buffer::enable();

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

		//cliffs
		//TODO:

		//shadows
		if (_show_shadows)
		{
			const auto pop_shader = s.shader;
			s.shader = _shader_shadows_lighting.get_shader();
			_quads.draw(t, _shadow_start, _grid_start - _shadow_start, s);
			s.shader = pop_shader;
		}

		if (_show_grid && _grid_tex)
		{
			
			s.texture = &resources::texture_functions::get_sf_texture(_grid_tex);
			_quads.draw(t, _grid_start, _quads.size() - _grid_start, s);
		}

		depth_buffer::disable();
		return;
	}

	void mutable_terrain_map::set_world_rotation(const float rot)
	{
		auto t = sf::Transform{};
		t.rotate(sf::degrees(rot));
		auto& l = _local_bounds;
		auto t_rect = t.transformRect(sf::FloatRect{ {l.x, l.y}, {l.width, l.height} });

		auto y_offset = t_rect.top;
		auto y_range = t_rect.height - y_offset;

		auto matrix = sf::Glsl::Mat4{ t };

		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
		{
			shdr->set_uniform("rotation_matrix"sv, matrix);
			shdr->set_uniform("y_offset"sv, y_offset);
			shdr->set_uniform("y_range"sv, y_range);
		}
		return;
	}

	void mutable_terrain_map::set_sun_angle(float degrees)
	{
		_sun_angle = to_radians(std::clamp(degrees, 0.f, 180.f));
		_needs_apply = true;
	}

	void mutable_terrain_map::place_tile(const tile_position p, const resources::tile& t)
	{
		hades::place_tile(_map, p, t, *_settings);
		_needs_apply = true;
		return;
	}

	void mutable_terrain_map::place_terrain(const terrain_vertex_position p, const resources::terrain* t)
	{
		hades::place_terrain(_map, p, t, *_settings);
		_needs_apply = true;
		return;
	}

	void mutable_terrain_map::raise_terrain(const terrain_vertex_position p, const uint8_t amount)
	{
		hades::raise_terrain(_map, p, amount, _settings);
		_needs_apply = true;
		return;
	}

	void mutable_terrain_map::lower_terrain(const terrain_vertex_position p, const uint8_t amount)
	{
		hades::lower_terrain(_map, p, amount, _settings);
		_needs_apply = true;
		return;
	}

	//==================================//
	//		mutable_terrain_map			//
	//==================================//

	mutable_terrain_map2::mutable_terrain_map2() :
		_shader{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_id)) },
		_shader_debug_depth{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_debug_id)) },
		_shader_shadows_lighting{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_shadows_lighting)) }
	{
		_shared.settings = resources::get_terrain_settings();
	}

	mutable_terrain_map2::mutable_terrain_map2(terrain_map t) : mutable_terrain_map2{}
	{
		reset(std::move(t));
	}

	void mutable_terrain_map2::reset(terrain_map t)
	{
		const auto& tile_size = _shared.settings->tile_size;
		const auto& width = t.tile_layer.width;
		const auto& tiles = t.tile_layer.tiles;

		_shared.local_bounds = rect_float{
			0.f, 0.f,
			float_cast(width * tile_size),
			float_cast((tiles.size() / width) * tile_size)
		};

		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
			shdr->set_uniform("height_multiplier"sv, _show_height * 1.f);

		/*const auto terrain_size = static_cast<vector2<unsigned int>>(get_vertex_size(t));
		if (!_sunlight_table->create({ terrain_size.x, terrain_size.y }))
		{
			throw runtime_error{
				std::format("Unable to create texture for terrain shadowmap. Size was: ({}, {})"sv,
				terrain_size.x, terrain_size.y)
			};
		}

		_sunlight_table->setSmooth(true);*/

		//_shader_shadows_lighting.set_uniform("tile_size"sv, float_cast(tile_size));
		//_shader_shadows_lighting.set_uniform("world_size"sv, static_cast<vector2_float>(get_vertex_size(t)));

		_shared.map = std::move(t);

		set_world_rotation(0.f);

		//_needs_apply = true;
		return;
	}

	constexpr auto bad_chunk_size = std::numeric_limits<std::size_t>::max();

	void mutable_terrain_map2::set_world_region(rect_float r) noexcept
	{
		// expand visible area to cover for terrain height
		r.x -= 255.f;
		r.y -= 255.f;
		r.width += 255.f * 2.f;
		r.height += 255.f * 2.f;

		if (is_within(r, _shared.world_area))
			return;

		if (r != _shared.world_area)
		{
			_needs_update = true;
			_shared.world_area = r;
		}
		return;
	}

	/*void mutable_terrain_map2::set_chunk_size(std::size_t s) noexcept
	{
		const auto d = std::div(s, _settings->tile_size);
		const auto new_size = d.quot + d.rem;
		if (_chunk_size != new_size)
			_chunk_size = new_size;

		return;
	}*/

	void mutable_terrain_map2::set_height_enabled(bool b) noexcept
	{
		_show_height = b;
		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
			shdr->set_uniform("height_multiplier"sv, _show_height * 1.f);
		return;
	}

	struct map_tile
	{
		tile_position position;
		std::array<std::uint8_t, 4> height = {};
		resources::tile_size_t left;
		resources::tile_size_t top;
		std::uint8_t texture = {};
	};

	static void generate_layer(mutable_terrain_map2::shared_data& shared,
		std::vector<map_tile>& tile_buffer,	const tile_map& map,
		const rect_int terrain_area)
	{
		tile_buffer.clear();
		const auto map_size_tiles = get_size(shared.map.tile_layer);
		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto index = to_1d_index(pos, map_size_tiles.x);
			const auto& t = map.tiles[index];
			const auto tile = get_tile(map, t);
			if (!tile.tex)
				return;

			// get a index for the texture
			auto tex_index = std::numeric_limits<std::size_t>::max();
			const auto tex_size = size(shared.texture_table);
			for (auto j = std::size_t{}; j < tex_size; ++j)
			{
				if (shared.texture_table[j] == tile.tex)
				{
					tex_index = j;
					break;
				}
			}

			if (tex_index > tex_size)
			{
				shared.texture_table.emplace_back(tile.tex);
				tex_index = tex_size;
			}

			const auto corner_height = get_height_at(pos, shared.map);
			// add tile to the tile list
			tile_buffer.emplace_back(pos, corner_height, tile.left, tile.top, integer_cast<std::uint8_t>(tex_index));
			return;
			});

		if (empty(tile_buffer))
			return;

		std::ranges::sort(tile_buffer, {}, &map_tile::texture);
		
		auto current_region = mutable_terrain_map2::vertex_region{ 
			shared.quads.size(),
			{},
			tile_buffer.front().texture 
		};

		const auto tile_sizef = float_cast(shared.settings->tile_size);

		for (const auto& tile : tile_buffer)
		{
			if (tile.texture != current_region.texture_index)
			{
				current_region.end_index = shared.quads.size() - 1;
				shared.regions.emplace_back(current_region);
				current_region.start_index = shared.quads.size();
				current_region.texture_index = tile.texture;
			}

			const auto [x, y] = tile.position;
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
			colours.fill(colour{});

			for (auto i = std::size_t{}; i < size(tile.height); ++i)
				colours[i].r = tile.height[i];

			const auto quad = make_quad_animation(pos, { tile_sizef, tile_sizef }, tex_coords, colours);
			shared.quads.append(quad);
		}

		current_region.end_index = shared.quads.size();
		shared.regions.emplace_back(current_region);
		return;
	}

	// layer constants
	//constexpr auto grid_layer = 0;
	//constexpr auto cliff_layer = 2; // also the world edge skirt if we add one(might just add enough of a margin to hide the world edge)
	//constexpr auto tile_layer = 1;
	//constexpr auto protected_layers = std::max({ grid_layer, tile_layer, cliff_layer });

	static void generate_grid(mutable_terrain_map2::shared_data& shared,
		const rect_int terrain_area)
	{
		assert(shared.settings->grid_terrain);
		const auto& grid_tiles = resources::get_transitions(*shared.settings->grid_terrain, resources::transition_tile_type::all, *shared.settings);
		assert(!empty(grid_tiles));
		const auto& grid_tile = grid_tiles.front();
		shared.grid_tex = grid_tile.tex.get();
		shared.start_grid = shared.quads.size();

		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto map_size_tiles = get_size(shared.map.tile_layer);

		const auto tex_coords = rect_float{
				float_cast(grid_tile.left),
				float_cast(grid_tile.top),
				tile_sizef,
				tile_sizef
		};

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto corner_height = get_height_at(pos, shared.map);
			const auto position = vector2_float{
				float_cast(pos.x) * tile_sizef,
				float_cast(pos.y) * tile_sizef
			};

			std::array<colour, 4> colours;
			colours.fill(colour{});

			for (auto i = std::size_t{}; i < size(corner_height); ++i)
				colours[i].r = corner_height[i];

			const auto quad = make_quad_animation(position, { tile_sizef, tile_sizef }, tex_coords, colours);
			shared.quads.append(quad);
			return;
			});
		return;
	}

	/*static void generate_shadow_row(const std::uint8_t sun_rise, const terrain_map& map,
		const terrain_index_t row_begin, const terrain_index_t row_end, sf::Image& img, const unsigned int img_y)
	{
		assert(img.getSize().y >= img_y &&
			img.getSize().x >= integer_cast<unsigned int>(row_end - row_begin));
		const auto get_max_height = [&map](terrain_index_t i) noexcept {
			const auto [h1, h2, h3, h4] = get_height_at(i, map);
			return std::max({ h1, h2, h3, h4 });
			};

		auto x = unsigned int{};
		auto h = get_max_height(row_begin);

		img.setPixel({ x++, img_y }, sf::Color{ h, 0, 0, 255 });

		while (std::cmp_less(row_begin + x, row_end))
		{
			if (h < sun_rise)
				h = {};
			else
				h -= sun_rise;

			const auto h2 = get_max_height(row_begin + x);
			if (h2 > h)
				h = h2;

			img.setPixel({ x, img_y }, sf::Color{ h, 0, 0, 255 });
			++x;
		}
		return;
	}*/

	static std::array<std::uint8_t, 4> calculate_vert_shadow(tile_position p, const tile_position dir,
		const std::uint8_t sun_rise, const std::uint8_t sun_dist, const terrain_map& m,
		const tile_position map_size) noexcept
	{
		auto height = std::array<std::uint8_t, 4>{};
		for (auto i = std::size_t{}; i < sun_dist; ++i)
		{
			p += dir;
			if (!within_map(map_size, p))
				break;
			const auto h = get_height_at(p, m);
			const auto sub_h = sun_rise * i;

			for (auto j = std::size_t{}; j < 4; ++j)
			{
				if (std::cmp_greater(sub_h, h[j]))
					continue;
				height[j] = std::max(height[j], integer_cast<std::uint8_t>(h[j] - sub_h));
			}
		}
		return height;
	}

	static std::tuple<std::uint8_t, std::uint8_t> calculate_vertex_normal(const terrain_vertex_position&)
	{
		return {};
	}

	static void generate_lighting(mutable_terrain_map2::shared_data& shared,
		const rect_int terrain_area)
	{
		const auto& sun_angle_radian = shared.sun_angle_radians;
		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto sun_unit_vect = to_vector(pol_vector2_t<float>{ sun_angle_radian /*+ std::numbers::pi_v<float>*/, 1.f });
		const auto sun_step_mag = sun_unit_vect.x == 0.f ? 1 : std::sqrt(1 + std::pow(sun_unit_vect.y / sun_unit_vect.x, 2.f));
		const auto sun_vector = sun_unit_vect * sun_step_mag * tile_sizef;
		
		// difference in height over x
		const auto sun_rise = integral_clamp_cast<std::uint8_t>(std::abs(sun_vector.y));
		// max x difference that could effect height
		const auto sun_dist = sun_vector.y == 0.f ? std::uint8_t{} : integral_clamp_cast<std::uint8_t>(255.f / std::abs(sun_vector.y));
		const auto sun_hidden = sun_angle_radian > std::numbers::pi_v<float> || sun_angle_radian < 0.f;
		const auto sun_90 = sun_angle_radian == std::numbers::pi_v<float> / 2.f;
		const auto sun_left = sun_angle_radian > std::numbers::pi_v<float> / 2.f;

		const auto dir = [](bool left) noexcept {
			if (left)
				return tile_position{ -1, 0 };
			else
				return tile_position{ 1, 0 };
			}(sun_left);

		const auto map_size_tiles = get_size(shared.map.tile_layer);
		const auto map_width_verts = shared.map.tile_layer.width + 1;

		shared.start_lighting = shared.quads.size();

		// generate shadowmap for region
		// generate normalmap for region

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			//const auto index = to_1d_index(pos, map_size_tiles.x);
			const auto corner_height = get_height_at(pos, shared.map);
			const auto position = vector2_float{
				float_cast(pos.x) * tile_sizef,
				float_cast(pos.y) * tile_sizef
			};

			const auto shadow_height = [&]()->std::array<std::uint8_t, 4> {
				if (sun_90)
					return { 0, 0, 0, 0 };
				else if (sun_hidden)
					return { 255, 255, 255, 255 };
				else
					return calculate_vert_shadow(pos, dir, sun_rise, sun_dist, shared.map, map_size_tiles);
			}();

			std::array<colour, 4> colours;
			colours.fill(colour{});

			for (auto i = std::size_t{}; i < size(corner_height); ++i)
			{
				colours[i].r = corner_height[i];
				colours[i].g = shadow_height[i];
			}

			const auto quad = make_quad_colour({ position, { tile_sizef, tile_sizef } }, colours);
			shared.quads.append(quad);
			return;
		});

		return;

		// TODO: clean up type usage here, heightmap can be indexed with size_t
		//		but img can only be indexed with unsigned
		//const auto vert_size = get_vertex_size(map);

		//auto img = sf::Image{};
		//img.create(sf::Vector2u{ integer_cast<unsigned>(vert_size.x), integer_cast<unsigned>(vert_size.y) }, sf::Color::Black);

		//// find the y distance between tile_size x

		//for (auto i = terrain_index_t{}; i < vert_size.y; ++i)
		//{
		//	const auto row_begin = vert_size.x * i;
		//	generate_shadow_row(sun_rise, map, row_begin, row_begin + vert_size.x, img, integer_cast<unsigned int>(i));
		//}

		//return img;

		// shadows
		// TODO: set shadow strength based on the angle of the sun
		//_shadow_start = std::size(_quads);
		//const auto shadow_map = generate_shadowmap(_sun_angle, tile_sizef, _map);
		//assert(shadow_map.getSize() == _sunlight_table->getSize());
		//_sunlight_table->update(shadow_map);

		//_shadow_tile_info.clear();
		//std::ranges::transform(_info.tile_info, std::inserter(_shadow_tile_info, end(_shadow_tile_info)),
		//	[](const map_tile& m) noexcept {
		//		return shadow_tile{ m.index, m.height };
		//	});

		//const auto sun_vect = to_vector(basic_pol_vector<float, 3>{ 0.f, _sun_angle, 1.f });

		//for (const auto& tile : _shadow_tile_info)
		//{
		//	const auto [x, y] = to_2d_index(tile.index, width);
		//	const auto pos = vector2_float{
		//		float_cast(x) * tile_sizef,
		//		float_cast(y) * tile_sizef
		//	};

		//	constexpr auto top_left = enum_type(rect_corners::top_left),
		//		top_right = enum_type(rect_corners::top_right),
		//		//bottom_right = enum_type(rect_corners::bottom_right),
		//		bottom_left = enum_type(rect_corners::bottom_left);

		//	// calculate normal
		//	// we only need to calculate normal for one of the triangles
		//	// TODO: calculate normal for each  vertex, by adverageing the normals of adjacent tris
		//	const auto p1 = vector3<float>{ pos.x, pos.y, float_cast(tile.height[top_left]) };
		//	const auto p2 = vector3<float>{ pos.x + tile_sizef, pos.y, float_cast(tile.height[top_right]) };
		//	const auto p3 = vector3<float>{ pos.x, pos.y + tile_sizef, float_cast(tile.height[bottom_left]) };

		//	const auto u = p2 - p1;
		//	const auto v = p3 - p1;

		//	const auto normal = vector::unit(vector::cross(u, v));
		//	const auto dot = vector::dot(normal, sun_vect);
		//	const auto light_val = dot < 0.f ? integral_clamp_cast<std::uint8_t>(dot * 255.f * -1.f) : std::uint8_t{};

		//	std::array<colour, 4> colours;
		//	colours.fill(colour{ {}, light_val });

		//	for (auto i = std::size_t{}; i < size(tile.height); ++i)
		//		colours[i].r = tile.height[i];

		//	const auto quad = make_quad_colour({ pos, { tile_sizef, tile_sizef } }, colours);
		//	_quads.append(quad);
		//}
	}

	static void generate_chunk(mutable_terrain_map2::shared_data& shared,
		std::vector<map_tile>& tile_buffer,	const std::size_t,
		const rect_int terrain_area)
	{
		//generate terrain layers
		const auto s = size(shared.map.terrain_layers);
		for (auto i = s; i > 0; --i)
			generate_layer(shared, tile_buffer, shared.map.terrain_layers[i - 1], terrain_area);

		//generate tiled layer
		if (!empty(shared.map.tile_layer.tiles))
		{
			generate_layer(shared, tile_buffer, shared.map.tile_layer, terrain_area);
			// generate cliffs quad_layer_count * (s + 1)
			generate_lighting(shared, terrain_area);
			generate_grid(shared, terrain_area);
		}

		return;
	}

	void mutable_terrain_map2::apply()
	{
		if (!_needs_update)
			return;

		_shared.regions.clear();
		_shared.quads.clear();

		const auto tiled_area = static_cast<vector2_int>(size(_shared.world_area) / float_cast(_shared.settings->tile_size));

		auto tile_buffer = std::vector<hades::map_tile>{};
		tile_buffer.reserve(integer_cast<std::size_t>(tiled_area.x) * integer_cast<std::size_t>(tiled_area.y));

		const auto tiled_start = static_cast<vector2_int>(position(_shared.world_area) / float_cast(_shared.settings->tile_size));

		generate_chunk(_shared, tile_buffer, 0, { tiled_start, tiled_area });
		/*if (!_needs_apply)
			return;

		_info.tile_info.clear();
		_info.texture_table.clear();*/

		

		/*std::ranges::sort(_info.tile_info, {}, &map_tile::texture);

		std::ranges::stable_sort(_info.tile_info, std::greater{}, &map_tile::layer);

		_shared.quads.clear();
		_shared.quads.reserve(size(_info.tile_info));

		assert(_settings);
		const auto tile_size = _settings->tile_size;
		const auto tile_sizef = float_cast(tile_size);
		const auto width = _shared.map.tile_layer.width;

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
			colours.fill(colour{});

			for (auto i = std::size_t{}; i < size(tile.height); ++i)
				colours[i].r = tile.height[i];

			const auto quad = make_quad_animation(pos, { tile_sizef, tile_sizef }, tex_coords, colours);
			_shared.quads.append(quad);
		}*/

		// cliffs begin
		//Cliffs
		// add to quadmap and store start index of cliffs

		// shadows
		// TODO: set shadow strength based on the angle of the sun
		//_shadow_start = std::size(_quads);
		//const auto shadow_map = generate_shadowmap(_sun_angle, tile_sizef, _map);
		//assert(shadow_map.getSize() == _sunlight_table->getSize());
		//_sunlight_table->update(shadow_map);

		//_shadow_tile_info.clear();
		//std::ranges::transform(_info.tile_info, std::inserter(_shadow_tile_info, end(_shadow_tile_info)),
		//	[](const map_tile& m) noexcept {
		//		return shadow_tile{ m.index, m.height };
		//	});

		//const auto sun_vect = to_vector(basic_pol_vector<float, 3>{ 0.f, _sun_angle, 1.f });

		//for (const auto& tile : _shadow_tile_info)
		//{
		//	const auto [x, y] = to_2d_index(tile.index, width);
		//	const auto pos = vector2_float{
		//		float_cast(x) * tile_sizef,
		//		float_cast(y) * tile_sizef
		//	};

		//	constexpr auto top_left = enum_type(rect_corners::top_left),
		//		top_right = enum_type(rect_corners::top_right),
		//		//bottom_right = enum_type(rect_corners::bottom_right),
		//		bottom_left = enum_type(rect_corners::bottom_left);

		//	// calculate normal
		//	// we only need to calculate normal for one of the triangles
		//	// TODO: calculate normal for each  vertex, by adverageing the normals of adjacent tris
		//	const auto p1 = vector3<float>{ pos.x, pos.y, float_cast(tile.height[top_left]) };
		//	const auto p2 = vector3<float>{ pos.x + tile_sizef, pos.y, float_cast(tile.height[top_right]) };
		//	const auto p3 = vector3<float>{ pos.x, pos.y + tile_sizef, float_cast(tile.height[bottom_left]) };

		//	const auto u = p2 - p1;
		//	const auto v = p3 - p1;

		//	const auto normal = vector::unit(vector::cross(u, v));
		//	const auto dot = vector::dot(normal, sun_vect);
		//	const auto light_val = dot < 0.f ? integral_clamp_cast<std::uint8_t>(dot * 255.f * -1.f) : std::uint8_t{};

		//	std::array<colour, 4> colours;
		//	colours.fill(colour{ {}, light_val });

		//	for (auto i = std::size_t{}; i < size(tile.height); ++i)
		//		colours[i].r = tile.height[i];

		//	const auto quad = make_quad_colour({ pos, { tile_sizef, tile_sizef } }, colours);
		//	_quads.append(quad);
		//}

		//_shader_shadows_lighting.set_uniform("sunlight_map"sv, &*_sunlight_table);

		// grid
		/*_grid_start = std::size(_quads);
		_grid_tex = generate_grid2(_map, *_settings, tile_sizef, _quads);*/

		_shared.quads.apply();
		_needs_update = false;
		return;
	}

	void mutable_terrain_map2::draw(sf::RenderTarget& t, const sf::RenderStates& states) const
	{
		assert(!_needs_update);

		auto s = states;
		s.shader = _debug_depth ? _shader_debug_depth.get_shader() : _shader.get_shader();
		const auto beg = begin(_info.tile_info);
		const auto ed = end(_info.tile_info);
		auto it = beg;

		depth_buffer::setup();
		depth_buffer::clear();
		depth_buffer::enable();

		for (const auto& region : _shared.regions)
		{
			assert(size(_shared.texture_table) > region.texture_index);
			s.texture = &resources::texture_functions::get_sf_texture(_shared.texture_table[region.texture_index].get());
			_shared.quads.draw(t, region.start_index, region.end_index - region.start_index, s);
		}

		/*while (it != ed)
		{
			auto tex = it->texture;
			auto index = distance(beg, it);
			while (it != ed && it->texture == tex)
				++it;

			auto last = distance(beg, it);
			s.texture = &resources::texture_functions::get_sf_texture(_shared.texture_table[tex].get());
			_shared.quads.draw(t, index, last - index, s);
		}*/

		//cliffs
		//TODO:

		//shadows
		if (_show_shadows)
		{
			const auto pop_shader = s.shader;
			s.shader = _shader_shadows_lighting.get_shader();
			_shared.quads.draw(t, _shared.start_lighting, _shared.start_grid - _shared.start_lighting, s);
			s.shader = pop_shader;
		}

		if (_show_grid && _shared.grid_tex)
		{
			s.texture = &resources::texture_functions::get_sf_texture(_shared.grid_tex);
			_shared.quads.draw(t, _shared.start_grid, _shared.quads.size() - _shared.start_grid, s);
		}

		depth_buffer::disable();
		return;
	}

	void mutable_terrain_map2::set_world_rotation(const float rot)
	{
		auto t = sf::Transform{};
		t.rotate(sf::degrees(rot));
		auto& l = _shared.local_bounds;
		auto t_rect = t.transformRect(sf::FloatRect{ {l.x, l.y}, {l.width, l.height} });

		auto y_offset = t_rect.top;
		auto y_range = t_rect.height - y_offset;

		auto matrix = sf::Glsl::Mat4{ t };

		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
		{
			shdr->set_uniform("rotation_matrix"sv, matrix);
			shdr->set_uniform("y_offset"sv, y_offset);
			shdr->set_uniform("y_range"sv, y_range);
		}
		return;
	}

	void mutable_terrain_map2::set_sun_angle(float degrees)
	{
		if (degrees == 0.f)
			degrees = 0.01f;

		_shared.sun_angle_radians = to_radians(degrees);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map2::place_tile(const tile_position p, const resources::tile& t)
	{
		hades::place_tile(_shared.map, p, t, *_shared.settings);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map2::place_terrain(const terrain_vertex_position p, const resources::terrain* t)
	{
		hades::place_terrain(_shared.map, p, t, *_shared.settings);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map2::raise_terrain(const terrain_vertex_position p, const uint8_t amount)
	{
		hades::raise_terrain(_shared.map, p, amount, _shared.settings);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map2::lower_terrain(const terrain_vertex_position p, const uint8_t amount)
	{
		hades::lower_terrain(_shared.map, p, amount, _shared.settings);
		_needs_update = true;
		return;
	}

	//==================================//
	//		  terrain_mini_map			//
	//==================================//

	void terrain_mini_map::set_size(const vector2_float s)
	{
		_size = s;
		_quad.clear();
		_quad.append(make_quad_animation({}, s, rect_float{ {}, s }));
		_quad.apply();
		return;
	}

	void terrain_mini_map::update(const terrain_map& map)
	{
		auto img = sf::Image{};
		img.create({ integral_cast<unsigned>(_size.x), integral_cast<unsigned>(_size.y) }, sf::Color::Black);
		//const auto& terrain = map.terrain_vertex;

		// fill img with map data
		// TODO:

		if (!_texture)
			_texture = std::make_unique<sf::Texture>();

		//auto success = _texture->loadFromImage(img);
		return;
	}

	void terrain_mini_map::draw(sf::RenderTarget& rt, const sf::RenderStates& state) const
	{
		auto s = state;
		s.transform *= getTransform();
		s.texture = _texture.get();
		rt.draw(_quad, s);
		return;
	}
}
