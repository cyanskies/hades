#include "hades/terrain_map.hpp"

#include <numbers>

#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/OpenGL.hpp"

#include "hades/draw_tools.hpp"
#include "hades/logging.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/sf_color.hpp"
#include "hades/shader.hpp"
#include "hades/table.hpp"
#include "hades/triangle_math.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

#define HADES_TERRAIN_USE_SIMPLE_NORMALS
#define HADES_TERRAIN_USE_BLENDED_NORMALS

#ifndef HADES_TERRAIN_USE_SIMPLE_NORMALS
	#ifndef HADES_TERRAIN_USE_BLENDED_NORMALS
		#define HADES_TERRAIN_USE_BLENDED_NORMALS
	#endif
#endif

const auto vertex_source = R"(
#version 120
uniform mat4 rotation_matrix;
uniform float height_multiplier;
varying float model_view_vertex_y;

void main()
{
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
})";

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

constexpr auto fragment_normal_colour = "gl_FragColor = texture2D(tex, gl_TexCoord[0].xy);";
constexpr auto fragment_depth_colour = "gl_FragColor = vec4(gl_FragDepth, gl_FragDepth, gl_FragDepth, 1.f);tex;";

constexpr auto shadow_fragment_source = R"(
#version 120
uniform float y_offset;
uniform float y_range;
// direction from ground to sun
uniform vec3 sun_direction;
varying float model_view_vertex_y;

const vec4 sun_colour = vec4(1.f, 1.f, .6f, .15f);
const vec4 shadow_col = vec4(0, 0, 0, 0.3f);
const float pi = 3.1415926535897932384626433832795;

void main()
{{
	// calculate fragment depth along y axis
	gl_FragDepth = 1.f - ((model_view_vertex_y - y_offset) / y_range);
	// shadow info
	float shadow_height = gl_Color.g;
	float frag_height = gl_Color.r;
	// lighting info
	float theta = gl_Color.b * 2.f * pi - pi; // [-pi, pi]
	float phi = gl_Color.a * pi; // [0, pi]
	float sin_phi = sin(phi);

	// interpolation can denomalise the origional normal
	vec3 denormal = vec3(cos(theta) * sin_phi, sin(theta) * sin_phi, cos(phi));
	vec3 normal = normalize(denormal);

	float cos_angle = clamp(dot(normal, sun_direction), 0.f, 1.f);
	vec4 light_col = vec4(mix(vec3(0.f, 0.f, 0.f), sun_colour.rgb, cos_angle), sun_colour.a);
	vec4 shadow_col = mix(vec4(0.f, 0.f, 0.f, 0.f) , shadow_col, float(frag_height < shadow_height));
	// frag colour
	{}
}})";

constexpr auto shadow_calc_colour = "gl_FragColor = mix(light_col, shadow_col, float(frag_height < shadow_height));";
constexpr auto shadow_render_normals = "gl_FragColor = vec4(0.5f * (normal.rgb + vec3(1.f, 1.f, 1.f)), 1.f);";

static auto terrain_map_shader_id = hades::unique_zero;
static auto terrain_map_shader_debug_id = hades::unique_zero;
static auto terrain_map_shader_shadows_lighting = hades::unique_zero;
static auto terrain_map_shader_debug_lighting = hades::unique_zero;

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

			shdr_funcs::set_fragment(*shader, std::format(fragment_source, fragment_normal_colour));
			shdr_funcs::set_vertex(*shader, vertex_source);
			shdr_funcs::set_uniforms(*shader, uniforms); // copy uniforms(we move it later)
		}

		if (terrain_map_shader_debug_id == unique_zero)
		{ // make heightmap debug terrain shader
			terrain_map_shader_debug_id = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_debug_id);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(fragment_source, fragment_depth_colour));
			shdr_funcs::set_vertex(*shader, vertex_source);
			shdr_funcs::set_uniforms(*shader, std::move(uniforms));
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
				},{
					"sun_direction"s,
					resources::uniform{
						resources::uniform_type_list::vector3f,
						{} // (0.f, 0.f, 0.f)
					}
				}
		};

		// terrain shadow rendering shader
		if (terrain_map_shader_shadows_lighting == unique_zero)
		{
			terrain_map_shader_shadows_lighting = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_shadows_lighting);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(shadow_fragment_source, shadow_calc_colour));
			shdr_funcs::set_vertex(*shader, vertex_source);
			shdr_funcs::set_uniforms(*shader, uniforms);
		}

		// terrain shadow debug shader
		if (terrain_map_shader_debug_lighting == unique_zero)
		{
			terrain_map_shader_debug_lighting = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_debug_lighting);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(shadow_fragment_source, shadow_render_normals));
			shdr_funcs::set_vertex(*shader, vertex_source);
			shdr_funcs::set_uniforms(*shader, std::move(uniforms));
		}
	}

	//==================================//
	//		mutable_terrain_map			//
	//==================================//

	mutable_terrain_map::mutable_terrain_map() :
		_shader{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_id)) },
		_shader_debug_depth{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_debug_id)) },
		_shader_shadows_lighting{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_shadows_lighting)) },
		_shader_debug_lighting{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_debug_lighting)) }
	{
		_shared.settings = resources::get_terrain_settings();
	}

	mutable_terrain_map::mutable_terrain_map(const mutable_terrain_map&)
		: mutable_terrain_map{}
	{
		assert(false);
		throw logic_error{ "Never copy a mutable_terrain_map"};
	}

	mutable_terrain_map::mutable_terrain_map(terrain_map t) : mutable_terrain_map{}
	{
		reset(std::move(t));
	}

	void mutable_terrain_map::reset(terrain_map t)
	{
		const auto& tile_size = _shared.settings->tile_size;
		const auto width = get_tile_width(t);
		const auto& tiles = t.cliff_layer;

		_shared.local_bounds = rect_float{
			0.f, 0.f,
			float_cast(width * tile_size),
			float_cast((tiles.size() / width) * tile_size)
		};

		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting, &_shader_debug_lighting })
			shdr->set_uniform("height_multiplier"sv, _show_height * 1.f);

		_shared.map = std::move(t);

		set_world_rotation(0.f);
		_needs_update = true;
		// TODO: set sun angle
		return;
	}

	constexpr auto bad_chunk_size = std::numeric_limits<std::size_t>::max();

	void mutable_terrain_map::set_world_region(rect_float r) noexcept
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

	void mutable_terrain_map::set_height_enabled(bool b) noexcept
	{
		_show_height = b;
		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
			shdr->set_uniform("height_multiplier"sv, _show_height * 1.f);
		return;
	}

	static constexpr std::array<vector2_float, 4> make_cell_positions(
		const vector2_float pos, const float tile_size) noexcept
	{
		const auto quad = rect_float{ pos, { tile_size, tile_size } };
		const auto quad_right_x = quad.x + quad.width;
		const auto quad_bottom_y = quad.y + quad.height;
		return {
			vector2_float{ quad.x, quad.y },				//top left
			vector2_float{ quad_right_x, quad.y },			//top right
			vector2_float{ quad_right_x, quad_bottom_y },	//bottom right
			vector2_float{ quad.x, quad_bottom_y },			//bottom left
		};
	}

	struct tiny_normal {
		std::uint8_t theta,
			phi;
	};

	static poly_quad make_terrain_triangles(std::array<vector2_float, 4> pos,
		const cell_height_data& h, const terrain_map::triangle_type triangle_type,
		rect_float texture_quad, const std::array<std::uint8_t, 4> &shadow_h = {}, 
		const std::array<tiny_normal, 6> &normals = {}) noexcept
	{
		const auto& tex_left_x = texture_quad.x;
		const auto tex_right_x = texture_quad.x + texture_quad.width;
		const auto tex_bottom_y = texture_quad.y + texture_quad.height;

		auto p = std::array<vector2_float, 6>{};
		auto sf_col = std::array<sf::Color, 6>{};
		for (auto i = std::uint8_t{}; i < 6; ++i)
		{
			const auto q_index = enum_type(triangle_index_to_quad_index(i, triangle_type));
			p[i] = pos[q_index];

			const auto& normal = normals[i];
			sf_col[i] = sf::Color{
				h[q_index],
				shadow_h[q_index],
				normal.theta, // theta
				normal.phi };// phi
		}
		
		if (triangle_type == terrain_map::triangle_type::triangle_uphill)
		{
			return poly_quad{
				//first triangle
				sf::Vertex{ { p[0].x, p[0].y },	sf_col[0], { tex_left_x,	texture_quad.y	} },	//top left
				sf::Vertex{ { p[1].x, p[1].y },	sf_col[1], { tex_left_x,	tex_bottom_y	} },	//bottom left 
				sf::Vertex{ { p[2].x, p[2].y },	sf_col[2], { tex_right_x,	texture_quad.y	} },	//top right
				//second triangle
				sf::Vertex{ { p[3].x, p[3].y },	sf_col[3], { tex_right_x,	texture_quad.y	} },	//top right
				sf::Vertex{ { p[4].x, p[4].y },	sf_col[4], { tex_left_x,	tex_bottom_y	} },	//bottom left		
				sf::Vertex{ { p[5].x, p[5].y },	sf_col[5], { tex_right_x,	tex_bottom_y	} },	//bottom right
			};
		}
		else
		{
			// downhill triangulation, first triangle edges against the left and bottom sides of the quad
			//						second triangle edges against the right and top sides of the quad
			return poly_quad{
				//first triangle
				sf::Vertex{ { p[0].x, p[0].y },	sf_col[0], { texture_quad.x,	texture_quad.y } },	//top left
				sf::Vertex{ { p[1].x, p[1].y },	sf_col[1], { texture_quad.x,	tex_bottom_y } },	//bottom left	
				sf::Vertex{ { p[2].x, p[2].y },	sf_col[2], { tex_right_x,		tex_bottom_y } },	//bottom right
				//second triangle			  ,
				sf::Vertex{ { p[3].x, p[3].y },	sf_col[3], { texture_quad.x,	texture_quad.y } },	//top left
				sf::Vertex{ { p[4].x, p[4].y },	sf_col[4], { tex_right_x,		tex_bottom_y } },	//bottom right
				sf::Vertex{ { p[5].x, p[5].y },	sf_col[5], { tex_right_x,		texture_quad.y } },	//top right
			};
		}
	}

	struct map_tile
	{
		// TODO: typedef this as cell_position_data
		std::array<world_vector_t, 4> positions;
		cell_height_data height;
		resources::tile_size_t left;
		resources::tile_size_t top;

		using texture_index_t = std::uint8_t;
		texture_index_t texture = {};
	};

	static map_tile::texture_index_t get_texture_index(const resources::resource_link<resources::texture>& tex,
		std::vector<resources::resource_link<resources::texture>>& texture_table)
	{
		constexpr auto max_index = std::numeric_limits<map_tile::texture_index_t>::max();
		const auto table_size = size(texture_table);
		if (std::cmp_equal(table_size, max_index))
		{
			log_warning(std::format("Terrain map has exceeded the per map texture limit ({})", max_index));
			return max_index;
		}

		const auto tex_size = integer_clamp_cast<map_tile::texture_index_t>(table_size);
		for (auto j = map_tile::texture_index_t{}; j < tex_size; ++j)
		{
			if (texture_table[j] == tex)
				return j;
		}

		texture_table.emplace_back(tex);
		return tex_size;
	}

	// never pass an empty tile_buffer to this func
	static void make_layer_regions(mutable_terrain_map::shared_data& shared, 
		const std::span<const map_tile> tile_buffer, const float tile_sizef)
	{
		assert(!tile_buffer.empty());
		auto current_region = mutable_terrain_map::vertex_region{
			shared.quads.size(),
			{},
			tile_buffer.front().texture
		};

		for (const auto& tile : tile_buffer)
		{
			if (tile.texture != current_region.texture_index)
			{
				current_region.end_index = shared.quads.size() - 1;
				shared.regions.emplace_back(current_region);
				current_region.start_index = shared.quads.size();
				current_region.texture_index = tile.texture;
			}

			const auto tex_coords = rect_float{
				float_cast(tile.left),
				float_cast(tile.top),
				tile_sizef,
				tile_sizef
			};

			const auto tile_pos = tile_position{
				integral_clamp_cast<tile_position::value_type>(tile.positions[0].x / tile_sizef),
				integral_clamp_cast<tile_position::value_type>(tile.positions[0].y / tile_sizef)
			};
			const auto type = pick_triangle_type(tile_pos);
			const auto quad = make_terrain_triangles(tile.positions, tile.height, type, tex_coords);
			shared.quads.append(quad);
		}

		current_region.end_index = shared.quads.size();
		shared.regions.emplace_back(current_region);
		return;
	}

	static void generate_layer(mutable_terrain_map::shared_data& shared,
		std::vector<map_tile>& tile_buffer,	const tile_map& map,
		const rect_int terrain_area)
	{
		tile_buffer.clear();
		tile_buffer.reserve(integer_clamp_cast<std::size_t>(terrain_area.x) * integer_clamp_cast<std::size_t>(terrain_area.y));

		const auto map_size_tiles = get_size(shared.map);
		const auto tile_sizef = float_cast(shared.settings->tile_size);
		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto& tile = get_tile_at(map, pos);
			if (!tile.tex)
				return;

			const auto [x, y] = pos;
			const auto pos_f = vector2_float{
				float_cast(x) * tile_sizef,
				float_cast(y) * tile_sizef
			};

			const auto triangle_height = get_height_for_cell(pos, shared.map, *shared.settings);
			const auto positions = make_cell_positions(pos_f, tile_sizef);
			// get a index for the texture
			const auto tex_index = get_texture_index(tile.tex, shared.texture_table);
			// add tile to the tile list
			tile_buffer.emplace_back(positions, triangle_height, tile.left, tile.top, tex_index);
			return;
		});

		if (empty(tile_buffer))
			return;

		std::ranges::sort(tile_buffer, {}, &map_tile::texture);
		make_layer_regions(shared, tile_buffer, tile_sizef);
		return;
	}

	static std::array<std::optional<map_tile>, 4> make_cliffs(const tile_position surface_pos, 
		const vector2_float world_pos_f,
		const cell_height_data surface_cell_height,
		const adjacent_cliffs cliffs,
		const float tile_sizef,
		const mutable_terrain_map::shared_data& shared)
	{
		constexpr auto tile_left = std::numeric_limits<resources::tile_size_t>::max();
		constexpr auto tile_top = tile_left;
		constexpr auto tex_index = std::numeric_limits<map_tile::texture_index_t>::max();

		auto out = std::array<std::optional<map_tile>, 4>{};

		// set invert to true for the bottom and left cliffs
		const auto make_cliff_height = [](auto&& top_height, auto&& bottom_height, const bool invert = false)->cell_height_data {
			return {
				top_height[static_cast<std::size_t>(invert)],
				top_height[static_cast<std::size_t>(!invert)],
				bottom_height[static_cast<std::size_t>(!invert)],
				bottom_height[static_cast<std::size_t>(invert)] 
			};
		};

		// top cliff
		if (cliffs[enum_type(rect_edges::top)])
		{
			// get triangle_height for tile above,
			// mix and match the hight and position values to create the correct tile
			const auto tile_above = surface_pos - tile_position{ 0, 1 };
			// cliff edges[top] should be 0 if the tile was outside the world.
			assert(within_world(tile_above, get_size(shared.map)));

			const auto top_height = get_height_for_cell(tile_above, shared.map, *shared.settings);
			const auto top_height_bottom_edge = get_height_for_bottom_edge(top_height);
			const auto surface_height_top_edge = get_height_for_top_edge(surface_cell_height);

			const auto new_height = make_cliff_height(top_height_bottom_edge, surface_height_top_edge);

			const auto new_positions = std::array<world_vector_t, 4>{
				world_vector_t{ world_pos_f.x, world_pos_f.y },
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y },
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y },
				world_vector_t{ world_pos_f.x, world_pos_f.y },
			};

			out[enum_type(rect_edges::top)] = map_tile{ new_positions, new_height, tile_left, tile_top, tex_index };
		}

		// right cliff
		if (cliffs[enum_type(rect_edges::right)])
		{
			// get triangle_height for tile above,
			// mix and match the hight and position values to create the correct tile
			const auto tile_right = surface_pos + tile_position{ 1, 0 };
			// cliff edges[top] should be 0 if the tile was outside the world.
			assert(within_world(tile_right, get_size(shared.map)));

			const auto right_height = get_height_for_cell(tile_right, shared.map, *shared.settings);
			const auto right_height_left_edge = get_height_for_left_edge(right_height);
			const auto surface_height_right_edge = get_height_for_right_edge(surface_cell_height);

			const auto new_height = make_cliff_height(right_height_left_edge, surface_height_right_edge);

			const auto new_positions = std::array<world_vector_t, 4>{
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y },
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y + tile_sizef },
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y + tile_sizef },
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y },
			};

			out[enum_type(rect_edges::right)] = map_tile{ new_positions, new_height, tile_left, tile_top, tex_index };
		}

		// bottom cliff
		if (cliffs[enum_type(rect_edges::bottom)])
		{
			// get triangle_height for tile above,
			// mix and match the hight and position values to create the correct tile
			const auto tile_right = surface_pos + tile_position{ 0, 1 };
			// cliff edges[top] should be 0 if the tile was outside the world.
			assert(within_world(tile_right, get_size(shared.map)));

			const auto right_height = get_height_for_cell(tile_right, shared.map, *shared.settings);
			const auto bottom_height_top_edge = get_height_for_top_edge(right_height);
			const auto surface_height_bottom_edge = get_height_for_bottom_edge(surface_cell_height);

			const auto new_height = make_cliff_height(bottom_height_top_edge, surface_height_bottom_edge, true);

			const auto new_positions = std::array<world_vector_t, 4>{
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y + tile_sizef },
				world_vector_t{ world_pos_f.x, world_pos_f.y + tile_sizef },
				world_vector_t{ world_pos_f.x, world_pos_f.y + tile_sizef },
				world_vector_t{ world_pos_f.x + tile_sizef, world_pos_f.y + tile_sizef },
			};

			out[enum_type(rect_edges::bottom)] = map_tile{ new_positions, new_height, tile_left, tile_top, tex_index };
		}

		// left cliff
		if (cliffs[enum_type(rect_edges::left)])
		{
			// get triangle_height for tile above,
			// mix and match the hight and position values to create the correct tile
			const auto tile_right = surface_pos - tile_position{ 1, 0 };
			// cliff edges[top] should be 0 if the tile was outside the world.
			assert(within_world(tile_right, get_size(shared.map)));

			const auto right_height = get_height_for_cell(tile_right, shared.map, *shared.settings);
			const auto left_height_right_edge = get_height_for_right_edge(right_height);
			const auto surface_height_left_edge = get_height_for_left_edge(surface_cell_height);

			const auto new_height = make_cliff_height(left_height_right_edge, surface_height_left_edge, true);

			const auto new_positions = std::array<world_vector_t, 4>{
				world_vector_t{ world_pos_f.x, world_pos_f.y + tile_sizef },
				world_vector_t{ world_pos_f.x, world_pos_f.y },
				world_vector_t{ world_pos_f.x, world_pos_f.y },
				world_vector_t{ world_pos_f.x, world_pos_f.y + tile_sizef },
			};

			out[enum_type(rect_edges::left)] = map_tile{ new_positions, new_height, tile_left, tile_top, tex_index };
		}

		return out;
	}

	static void generate_cliffs(mutable_terrain_map::shared_data& shared,
		std::vector<map_tile>& tile_buffer,	const rect_int terrain_area)
	{
		if (!shared.map.terrainset || !shared.map.terrainset->cliff_type)
		{
			log_error("Missing cliff data"sv);
			return;
		}

		tile_buffer.clear();
		const auto map_size_tiles = get_size(shared.map);
		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto cliff_type = shared.map.terrainset->cliff_type.get();
		const auto& empty_tile = resources::get_empty_tile(*shared.settings);

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto cliffs = get_adjacent_cliffs(pos, shared.map);
			const auto cliff_corners = get_cliff_corners(pos, shared.map);

			const auto world_pos = to_pixels(pos, shared.settings->tile_size);
			const auto world_pos_f = static_cast<vector2_float>(world_pos);
			const auto surface_triangle_height = get_height_for_cell(pos, shared.map, *shared.settings);
			const auto surface_positions = make_cell_positions(world_pos_f, tile_sizef);

			using cliff_layout = terrain_map::cliff_layer_layout;
			const auto base_index = integer_cast<std::size_t>(to_tile_index(pos, map_size_tiles.x) * enum_type(cliff_layout::multiplier));
			const auto surface_index = base_index + enum_type(cliff_layout::surface);

			assert(surface_index < size(shared.map.cliff_tiles.tiles));
			if (const auto& tile = get_tile(shared.map.cliff_tiles, shared.map.cliff_tiles.tiles[surface_index]); tile != empty_tile)
			{
				const auto tex_index = get_texture_index(tile.tex, shared.texture_table);
				tile_buffer.emplace_back(surface_positions, surface_triangle_height, tile.left, tile.top, tex_index);
			}

			const auto cliff_faces = make_cliffs(pos, world_pos_f,
				surface_triangle_height, cliffs, tile_sizef, shared);
			for(auto i = cliff_layout::top; i < cliff_layout::surface; i = next(i))
			{
				const auto& cliff_face = cliff_faces[enum_type(i)];
				if (!cliff_face)
					continue;
				map_tile c = *cliff_face;
				const auto& tile = get_tile(shared.map.cliff_tiles, shared.map.cliff_tiles.tiles[base_index + enum_type(i)]);
				if (tile != empty_tile)
				{
					const auto tex_index = get_texture_index(tile.tex, shared.texture_table);
					c.left = tile.left;
					c.top = tile.top;
					c.texture = tex_index;
					tile_buffer.emplace_back(c);
				}
			}
			return;
		});

		if (empty(tile_buffer))
			return;

		std::ranges::sort(tile_buffer, {}, &map_tile::texture);
		make_layer_regions(shared, tile_buffer, tile_sizef);

		return;
	}

	static void generate_grid(mutable_terrain_map::shared_data& shared,
		const rect_int terrain_area)
	{
		if (!shared.settings->grid_terrain)
		{
			log_error("Grid terrain missing"sv);
			return;
		}

		const auto& grid_tiles = resources::get_transitions(*shared.settings->grid_terrain, resources::transition_tile_type::all, *shared.settings);
		assert(!empty(grid_tiles));
		const auto& grid_tile = grid_tiles.front();
		shared.grid_tex = grid_tile.tex.get();

		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto map_size_tiles = get_size(shared.map);

		const auto tex_coords = rect_float{
				float_cast(grid_tile.left),
				float_cast(grid_tile.top),
				tile_sizef,
				tile_sizef
		};

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto position = vector2_float{
				float_cast(pos.x) * tile_sizef,
				float_cast(pos.y) * tile_sizef
			};

			const auto triangle_height = get_height_for_cell(pos, shared.map, *shared.settings);
			const auto type = pick_triangle_type(pos);
			const auto p = make_cell_positions(position, tile_sizef);
			const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords);

			shared.quads.append(quad);
			return;
			});
		return;
	}

	static void generate_ramp(mutable_terrain_map::shared_data& shared,
		const rect_int terrain_area)
	{
		if (!shared.settings->ramp_overlay_terrain)
		{
			log_error("Ramp debug terrain missing"sv);
			return;
		}
		
		const auto& top_tiles = resources::get_transitions(*shared.settings->ramp_overlay_terrain, resources::transition_tile_type::top_left_right, *shared.settings);
		const auto& right_tiles = resources::get_transitions(*shared.settings->ramp_overlay_terrain, resources::transition_tile_type::top_right_bottom_right, *shared.settings);
		const auto& bottom_tiles = resources::get_transitions(*shared.settings->ramp_overlay_terrain, resources::transition_tile_type::bottom_left_right, *shared.settings);
		const auto& left_tiles = resources::get_transitions(*shared.settings->ramp_overlay_terrain, resources::transition_tile_type::top_left_bottom_left, *shared.settings);

		if (top_tiles.empty() || right_tiles.empty() || bottom_tiles.empty() || left_tiles.empty())
		{
			log_error("Missing some tile types for ramp debug terrain"sv);
			return;
		}

		const resources::tile &top = top_tiles.front(),
			&right = right_tiles.front(), 
			&bottom = bottom_tiles.front(),
			&left = left_tiles.front();

		if (top.tex != right.tex ||
			top.tex != bottom.tex ||
			top.tex != left.tex)
		{
			log_error("Ramp debug terrain tiles must all use the same texture"sv);
			return;
		}

		shared.ramp_tex = top.tex.get();

		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto map_size_tiles = get_size(shared.map);

		const auto tex_coords_top = rect_float{
				float_cast(top.left),
				float_cast(top.top),
				tile_sizef,
				tile_sizef
		};

		const auto tex_coords_right = rect_float{
				float_cast(right.left),
				float_cast(right.top),
				tile_sizef,
				tile_sizef
		};

		const auto tex_coords_bottom = rect_float{
				float_cast(bottom.left),
				float_cast(bottom.top),
				tile_sizef,
				tile_sizef
		};

		const auto tex_coords_left = rect_float{
				float_cast(left.left),
				float_cast(left.top),
				tile_sizef,
				tile_sizef
		};

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto index = to_tile_index(pos, map_size_tiles.x);
			assert(index < std::size(shared.map.ramp_layer));
			const auto ramp = std::bitset<4>{ shared.map.ramp_layer[index] };

			if (ramp.none())
				return;

			const auto position = vector2_float{
				float_cast(pos.x) * tile_sizef,
				float_cast(pos.y) * tile_sizef
			};

			const auto triangle_height = get_height_for_cell(pos, shared.map, *shared.settings);
			const auto type = pick_triangle_type(pos);
			const auto p = make_cell_positions(position, tile_sizef);

			if (ramp.test(enum_type(rect_edges::top)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_top);
				shared.quads.append(quad);
			}

			if (ramp.test(enum_type(rect_edges::right)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_right);
				shared.quads.append(quad);
			}

			if (ramp.test(enum_type(rect_edges::bottom)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_bottom);
				shared.quads.append(quad);
			}

			if (ramp.test(enum_type(rect_edges::left)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_left);
				shared.quads.append(quad);
			}

			return;
			});
		return;
	}

	static void generate_cliff_debug(mutable_terrain_map::shared_data& shared,
		const rect_int terrain_area)
	{
		if (!shared.settings->cliff_overlay_terrain)
		{
			log_error("Cliff debug terrain missing"sv);
			return;
		}

		const auto& top_tiles = resources::get_transitions(*shared.settings->cliff_overlay_terrain, resources::transition_tile_type::top_left_right, *shared.settings);
		const auto& right_tiles = resources::get_transitions(*shared.settings->cliff_overlay_terrain, resources::transition_tile_type::top_right_bottom_right, *shared.settings);
		const auto& bottom_tiles = resources::get_transitions(*shared.settings->cliff_overlay_terrain, resources::transition_tile_type::bottom_left_right, *shared.settings);
		const auto& left_tiles = resources::get_transitions(*shared.settings->cliff_overlay_terrain, resources::transition_tile_type::top_left_bottom_left, *shared.settings);

		if (top_tiles.empty() || right_tiles.empty() || bottom_tiles.empty() || left_tiles.empty())
		{
			log_error("Missing some tile types for cliff debug terrain"sv);
			return;
		}

		const resources::tile& top = top_tiles.front(),
			& right = right_tiles.front(),
			& bottom = bottom_tiles.front(),
			& left = left_tiles.front();

		if (top.tex != right.tex ||
			top.tex != bottom.tex ||
			top.tex != left.tex)
		{
			log_error("Cliff debug terrain tiles must all use the same texture"sv);
			return;
		}

		shared.cliff_debug_tex = top.tex.get();

		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto map_size_tiles = get_size(shared.map);

		const auto tex_coords_top = rect_float{
				float_cast(top.left),
				float_cast(top.top),
				tile_sizef,
				tile_sizef
		};

		const auto tex_coords_right = rect_float{
				float_cast(right.left),
				float_cast(right.top),
				tile_sizef,
				tile_sizef
		};

		const auto tex_coords_bottom = rect_float{
				float_cast(bottom.left),
				float_cast(bottom.top),
				tile_sizef,
				tile_sizef
		};

		const auto tex_coords_left = rect_float{
				float_cast(left.left),
				float_cast(left.top),
				tile_sizef,
				tile_sizef
		};

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto cliffs = get_adjacent_cliffs(pos, shared.map);
			if (cliffs.none())
				return;

			const auto world_pos = to_pixels(pos, shared.settings->tile_size);
			const auto world_pos_f = static_cast<vector2_float>(world_pos);
			const auto triangle_height = get_height_for_cell(pos, shared.map, *shared.settings);
			const auto type = pick_triangle_type(pos);
			const auto p = make_cell_positions(world_pos_f, tile_sizef);

			if (cliffs.test(enum_type(rect_edges::top)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_top);
				shared.quads.append(quad);
			}

			if (cliffs.test(enum_type(rect_edges::right)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_right);
				shared.quads.append(quad);
			}

			if (cliffs.test(enum_type(rect_edges::bottom)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_bottom);
				shared.quads.append(quad);
			}

			if (cliffs.test(enum_type(rect_edges::left)))
			{
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords_left);
				shared.quads.append(quad);
			}
		});
	}

	// TODO: we'll probably need this to draw units
	/*static float bilinear_height(const sf::Vector2f pos, const vector2_float tile_pos,
		const float tile_size, const std::array<std::uint8_t, 4>& h) noexcept
	{
		const auto x = (pos.x - tile_pos.x) / tile_size;
		const auto y = (pos.y - tile_pos.y) / tile_size;
		const auto tl_h = float_clamp_cast(h[enum_type(rect_corners::top_left)]);
		const auto tr_h = float_clamp_cast(h[enum_type(rect_corners::top_right)]);
		const auto bl_h = float_clamp_cast(h[enum_type(rect_corners::bottom_left)]);
		const auto br_h = float_clamp_cast(h[enum_type(rect_corners::bottom_right)]);

		const auto tl = tl_h * (1 - x) * (1 - y);
		const auto bl = bl_h * (1 - x) * y;
		const auto tr = tr_h * x * (1 - y);
		const auto br = br_h * x * y;
		const auto val = tl + tr + bl + br;
		return val;
	}*/

	static void generate_cliff_layer_debug(mutable_terrain_map::shared_data& shared,
		const rect_int terrain_area)
	{
		constexpr auto text_size = 13.f;
		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto half_tile = tile_sizef / 2.f;
		const auto map_size_tiles = get_size(shared.map);

		shared.gui.activate_context();
		auto text_renderer = shared.gui.make_text_renderer();
		auto out = std::vector<sf::Vertex>{};

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto cliff_layer = get_cliff_layer(pos, shared.map);
			const auto cliff_layer_str = std::format("{}", integer_cast<unsigned int>(cliff_layer));

			const auto world_pos = to_pixels(pos, shared.settings->tile_size);
			const auto world_pos_f = static_cast<vector2_float>(world_pos);
			const auto quad_height = get_height_for_cell(pos, shared.map, *shared.settings);
			
			text_renderer.new_frame();
			const auto txt_size = text_renderer.text_size(cliff_layer_str, text_size);
			const auto half_size = txt_size / 2.f;
			const auto final_pos = vector2_float{ world_pos_f.x + half_tile - half_size.x, world_pos_f.y + half_tile - half_size.y };
			text_renderer.draw_text(cliff_layer_str, text_size, final_pos);
			auto render = text_renderer.output_frame();
			assert(!shared.layer_tex || shared.layer_tex == render.texture);
			shared.layer_tex = render.texture;

			const auto max_height = std::ranges::max(quad_height);
			for (auto& vert : render.verts)
			{
				vert.color.r = integer_clamp_cast<std::uint8_t>(max_height + 1);
			}

			out.insert(end(out), begin(render.verts), end(render.verts));
		});

		auto ret = shared.cliff_layer_debug.create(size(out));
		if (!ret)
			log_error("Failed to create vertex buffer for cliff layer debug"sv);
			
		ret = shared.cliff_layer_debug.update(std::data(out));
		if (!ret)
			log_error("Failed to store cliff layer debug view in VertexBuffer"sv);
		return;
	}

	static constexpr std::array<std::uint8_t, 4> full_bright(const std::array<std::uint8_t, 4>&,
		const std::array<std::uint8_t, 4>&, const std::uint8_t) noexcept
	{
		constexpr auto min = std::uint8_t{};

		return {
			min, min, min, min
		};
	}

	static constexpr std::array<std::uint8_t, 4> full_dark(const std::array<std::uint8_t, 4>&,
		const std::array<std::uint8_t, 4>&, const std::uint8_t) noexcept
	{
		constexpr auto max = std::numeric_limits<std::uint8_t>::max();

		return {
			max, max, max, max
		};
	}

	static constexpr std::array<std::uint8_t, 4> push_shadows_from_left(const std::array<std::uint8_t, 4>& current,
		const std::array<std::uint8_t, 4>& next, const std::uint8_t sun_rise) noexcept
	{
		/*const auto l0 = current[1];
		const auto l3 = current[2];
		const auto l1 = l0 < sun_rise ? 0 : l0 - sun_rise;
		const auto l2 = l3 < sun_rise ? 0 : l3 - sun_rise;

		const auto n1 = next[0] < sun_rise ? 0 : next[0] - sun_rise;
		const auto n2 = next[3] < sun_rise ? 0 : next[3] - sun_rise;

		return {
			std::max(l0, next[0]),
			integer_cast<std::uint8_t>(std::max(l1, n1)),
			integer_cast<std::uint8_t>(std::max(l2, n2)),
			std::max(l3, next[3])
		};*/

		const auto l_tl = current[enum_type(rect_corners::top_right)];
		const auto l_bl = current[enum_type(rect_corners::bottom_right)];

		const auto n_tl = next[enum_type(rect_corners::top_left)];
		const auto n_bl = next[enum_type(rect_corners::bottom_left)];

		const auto n_tr = integer_clamp_cast<std::uint8_t>(std::max({ n_tl - sun_rise, l_tl - sun_rise }));
		const auto n_br = integer_clamp_cast<std::uint8_t>(std::max({ l_bl - sun_rise, n_bl - sun_rise }));
		
		return {
			n_tr > next[enum_type(rect_corners::top_right)] ? std::max(n_tl, l_tl) : l_tl,
			n_tr,
			n_br,
			n_br > next[enum_type(rect_corners::bottom_right)] ? std::max(n_bl, l_bl) : l_bl,
		};
	}

	static constexpr std::array<std::uint8_t, 4> push_shadows_from_right(const std::array<std::uint8_t, 4>& current,
		const std::array<std::uint8_t, 4>& next, const std::uint8_t sun_rise) noexcept
	{
		const auto l_tr = current[enum_type(rect_corners::top_left)];
		const auto l_br = current[enum_type(rect_corners::bottom_left)];

		const auto n_tr = next[enum_type(rect_corners::top_right)];
		const auto n_br = next[enum_type(rect_corners::bottom_right)];

		const auto n_tl = integer_clamp_cast<std::uint8_t>(std::max({ n_tr - sun_rise, l_tr - sun_rise }));
		const auto n_bl = integer_clamp_cast<std::uint8_t>(std::max({ l_br - sun_rise, n_br - sun_rise }));

		return {
			n_tl,
			n_tl > next[enum_type(rect_corners::top_left)] ? std::max(n_tr, l_tr) : l_tr,
			n_bl > next[enum_type(rect_corners::bottom_left)] ? std::max(n_br, l_br) : l_br,
			n_bl,
		};
	}

	// TODO: move this to header so we can store the lighting table memory for reuse
	struct lighting_info
	{
		using quad_normals = std::array<vector3<float>, 2>;
		quad_normals tri_normals;
		std::array<std::optional<quad_normals>, 4> cliff_normals;
		std::array<std::uint8_t, 4> height;
		std::array<std::uint8_t, 4> shadow_height;
		terrain_map::triangle_type triangle_type;
	};

	static lighting_info::quad_normals calculate_quad_normals(const std::array<vector2_float, 4>& posf,
		const cell_height_data& h, const terrain_map::triangle_type type) noexcept
	{
		constexpr auto top_left = enum_type(rect_corners::top_left),
			top_right = enum_type(rect_corners::top_right),
			bottom_right = enum_type(rect_corners::bottom_right),
			bottom_left = enum_type(rect_corners::bottom_left);
		constexpr auto height_multiplier = 1.f;

		const auto& tl = posf[top_left];
		const auto& tr = posf[top_right];
		const auto& br = posf[bottom_right];
		const auto& bl = posf[bottom_left];

		std::array<basic_triangle<float, 3>, 2> triangles [[indeterminate]];
		if (type == terrain_map::triangle_type::triangle_uphill)
		{
			triangles[0] = {
				{ tl.x, tl.y, height_multiplier * float_cast(h[top_left])},
				{ bl.x, bl.y, height_multiplier * float_cast(h[bottom_left]) },
				{ tr.x, tr.y, height_multiplier * float_cast(h[top_right]) }
			};
			triangles[1] = {
				{ tr.x,	tr.y, height_multiplier * float_cast(h[top_right]) },
				{ bl.x, bl.y, height_multiplier * float_cast(h[bottom_left]) },
				{ br.x, br.y, height_multiplier * float_cast(h[bottom_right]) }
			};
		}
		else
		{
			triangles[0] = {
				{ tl.x, tl.y, height_multiplier * float_cast(h[top_left]) },
				{ bl.x, bl.y, height_multiplier * float_cast(h[bottom_left]) },
				{ br.x, br.y, height_multiplier * float_cast(h[bottom_right]) }
			};
			triangles[1] = {
				{ tl.x, tl.y, height_multiplier * float_cast(h[top_left]) },
				{ br.x, br.y, height_multiplier * float_cast(h[bottom_right]) },
				{ tr.x, tr.y, height_multiplier * float_cast(h[top_right]) }
			};
		}

		return {
			triangle_normal(triangles[0]),
			triangle_normal(triangles[1])
		};
	}

	static std::array<std::optional<lighting_info::quad_normals>, 4> calculate_cliff_normals(
		const tile_position surface_pos, const vector2_float posf, const cell_height_data& h,
		const float tile_sizef, const mutable_terrain_map::shared_data& shared)
	{
		const auto type = pick_triangle_type(surface_pos);
		auto out = std::array<std::optional<lighting_info::quad_normals>, 4>{};
		const auto adj_cliffs = get_adjacent_cliffs(surface_pos, shared.map);
		const auto cliffs = make_cliffs(surface_pos, posf, h, adj_cliffs, tile_sizef, shared);
		for (auto i = rect_edges::begin; i != rect_edges::end; i = next(i))
		{
			if (cliffs[enum_type(i)])
			{
				const auto& pos = cliffs[enum_type(i)]->positions;
				const auto& height = cliffs[enum_type(i)]->height;
				out[enum_type(i)] = calculate_quad_normals(pos, height, type);
			}
		}

		return out;
	}

	template<typename Func>
	static void calculate_lighting_tile_row(tile_position p, table<lighting_info>& table, const tile_position dir,
		const std::uint8_t sun_rise, const float tile_sizef, const std::uint32_t row_length,
		const mutable_terrain_map::shared_data& shared, Func push_shadows)
	{
		const auto max_val = [](auto a, auto b) {
			return std::max(a, b);
			};

		auto shadow_h = get_height_for_cell(p, shared.map, *shared.settings);
	
		for (auto i = std::uint32_t{}; i < row_length; ++i)
		{
			const auto h = get_height_for_cell(p, shared.map, *shared.settings);
			const auto type = pick_triangle_type(p);
			
			shadow_h = std::invoke(push_shadows, shadow_h, h, sun_rise);

			// TODO: to_world_vector
			const auto posf = vector2_float{
				float_cast(p.x) * tile_sizef,
				float_cast(p.y) * tile_sizef
			};

			const auto cell_positions = make_cell_positions(posf, tile_sizef);
			const auto surface_normals = calculate_quad_normals(cell_positions, h, type);
			const auto cliff_normals = calculate_cliff_normals(p, posf, h, tile_sizef, shared);
			table[p] = lighting_info{ surface_normals, cliff_normals, h, shadow_h, type };

			p += dir;
		}
		return;
	}

	static constexpr tiny_normal mean_vector(const std::span<vector3<float>> vects) noexcept
	{
		const auto n = float_clamp_cast(std::ranges::distance(vects));
		const auto mean = std::reduce(begin(vects), end(vects), vector3<float>{}, std::plus{}) / n;
		const auto pol = vector::unit(to_pol_vector(mean));

		// returns the normal for the left triangle and then right triangle
		const auto to_normal = [](const basic_pol_vector<float, 3> pol) noexcept->tiny_normal {
			constexpr auto pi = std::numbers::pi_v<float>;
			const auto theta = remap(pol.theta, -pi, pi, 0.f, 255.f);
			const auto phi = remap(pol.phi, 0.f, pi, 0.f, 255.f);
			return { integral_cast<std::uint8_t>(theta), integral_cast<std::uint8_t>(phi) };
			};

		return to_normal(pol);
	}

	static tiny_normal calculate_normal_for_vertex(const tile_position tile,
		const terrain_vertex_position& v, const table<lighting_info>& table,
		const lighting_info& centre, const terrain_map& map, const tile_position& world_size)
	{
		constexpr auto default_normal = vector3<float>{ 0.f, 0.f, 1.f };
		auto normals = std::array<vector3<float>, 8>{};
		auto count = std::uint8_t{};

		/*for_each_safe_adjacent_corner(map, v, [](const auto &&tile_pos, const auto &&corner) {
			using enum rect_corners;
			switch (corner)
			{
			case top_left:
			case top_right:
			case bottom_right:
			case bottom_left:
			}
			});*/

		return mean_vector({ begin(normals), next(begin(normals), count) });
	}

	//static std::array<tiny_normal, 6> calculate_vertex_normal(const tile_position& p,
	//	const table<lighting_info>& table, const tile_position& world_size)
	//{
	//	const auto get_cell = [&](const tile_position& p) noexcept -> lighting_info {
	//		if (!within_world(p, world_size))
	//			return lighting_info{};

	//		return table[p];
	//	};
	//	
	//	const auto vertex = to_vertex_positions(p);
	//	const auto& centre = table[p];
	//	//const auto tl = calculate_normal_for_vertex(p, vertex[enum_type(rect_corners::top_left)], table, centre, world_size);

	//	//// NOTE: tiles (x is 'centre'
	//	////			Top row:	0, 1, 2
	//	////			Middle row: 3, x, 4
	//	////			bottom row: 5, 6, 7
	//	//const auto tiles = std::array{
	//	//	get_cell(p + tile_position{ -1, -1 }), // 0
	//	//	get_cell(p + tile_position{  0, -1 }), // 1
	//	//	get_cell(p + tile_position{  1, -1 }), // 2
	//	//	get_cell(p + tile_position{ -1,  0 }), // 3
	//	//	get_cell(p + tile_position{  1,  0 }), // 4
	//	//	get_cell(p + tile_position{ -1,  1 }), // 5
	//	//	get_cell(p + tile_position{  0,  1 }), // 6
	//	//	get_cell(p + tile_position{  1,  1 }), // 7
	//	//};

	//	//constexpr auto top_left = 0, top = 1, top_right = 2,
	//	//				left = 3, right = 4, 
	//	//				bottom_left = 5, bottom = 6, bottom_right = 7;

	//	//// TODO: triangle type
	//	//// average each of the contributing normals
	//	//const auto tl = mean_vector(std::array{ centre.tri_normals[0],
	//	//	tiles[top_left].tri_normals[1],
	//	//	tiles[top].tri_normals[0], tiles[top].tri_normals[1],
	//	//	tiles[left].tri_normals[0], tiles[left].tri_normals[1] });
	//	//const auto tr = mean_vector(std::array{ centre.tri_normals[0], centre.tri_normals[1],
	//	//	tiles[top].tri_normals[1], 
	//	//	tiles[top_right].tri_normals[0], tiles[top_right].tri_normals[1],
	//	//	tiles[right].tri_normals[0] });
	//	//const auto br = mean_vector(std::array{ centre.tri_normals[1] ,
	//	//	tiles[right].tri_normals[0], tiles[right].tri_normals[1],
	//	//	tiles[bottom_right].tri_normals[0],
	//	//	tiles[bottom].tri_normals[0], tiles[bottom].tri_normals[1]  });
	//	//const auto bl = mean_vector(std::array{ centre.tri_normals[0], centre.tri_normals[1],
	//	//	tiles[bottom].tri_normals[0],
	//	//	tiles[bottom_left].tri_normals[0], tiles[bottom_left].tri_normals[1],
	//	//	tiles[left].tri_normals[1] });

	//	if (centre.triangle_type == terrain_map::triangle_uphill)
	//	{
	//		return {
	//			tl, bl, tr,
	//			tr, bl, br
	//		};
	//	}
	//	else
	//	{
	//		return {
	//			tl, bl, br,
	//			tl, br, tr
	//		};
	//	}
	//}

	static std::array<tiny_normal, 6> calc_simple_normal(const lighting_info::quad_normals& tri_normals) noexcept
	{
		const auto to_normal = [](const vector3<float> vec)->tiny_normal {
			const auto pol = vector::unit(to_pol_vector(vec));
			constexpr auto pi = std::numbers::pi_v<float>;
			const auto theta = remap(pol.theta, -pi, pi, 0.f, 255.f);
			const auto phi = remap(pol.phi, 0.f, pi, 0.f, 255.f);
			return { integral_cast<std::uint8_t>(theta), integral_cast<std::uint8_t>(phi) };
		};

		return {
			to_normal(tri_normals[0]),
			to_normal(tri_normals[0]),
			to_normal(tri_normals[0]),
			to_normal(tri_normals[1]),
			to_normal(tri_normals[1]),
			to_normal(tri_normals[1])
		};
	}

	static void generate_lighting(mutable_terrain_map::shared_data& shared,
		const rect_int terrain_area)
	{
		const auto& sun_angle_radian = shared.sun_angle_radians;
		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto sun_unit_vect = to_vector(pol_vector2_t<float>{ sun_angle_radian, 1.f });
		const auto slope = sun_unit_vect.y / sun_unit_vect.x;
		const auto next_y = (sun_unit_vect.y * slope * (tile_sizef - sun_unit_vect.x));

		// difference in height over x
		const auto sun_rise = integral_clamp_cast<std::uint8_t>(std::abs(next_y));
		// max x difference that could effect height
		const auto sun_dist = next_y == 0.f ? std::uint8_t{} : integral_clamp_cast<std::uint8_t>(255.f / std::abs(next_y));
		const auto sun_hidden = sun_angle_radian > std::numbers::pi_v<float> || sun_angle_radian < 0.f;
		const auto sun_90 = sun_angle_radian == std::numbers::pi_v<float> / 2.f;
		const auto sun_left = sun_angle_radian > std::numbers::pi_v<float> / 2.f;

		const auto dir = [](bool left) constexpr noexcept {
			if (!left)
				return tile_position{ -1, 0 };
			else
				return tile_position{ 1, 0 };
			}(sun_left);

		const auto map_size_tiles = get_size(shared.map);
		
		auto table_area = terrain_area;
		table_area.x = std::max(table_area.x - sun_dist, 0);
		table_area.y = std::max(table_area.y - 1, 0);
		table_area.width = std::min(table_area.width + sun_dist, map_size_tiles.x);
		table_area.height = std::min(table_area.height, map_size_tiles.y);

		auto light_table = table<lighting_info>{ position(table_area), size(table_area), {} };
		// iterate over tiles
		const auto row_length = table_area.width;

		const auto shadow_func = [&]() noexcept {
			if (sun_hidden)
				return full_dark;
			else if (sun_90)
				return full_bright;
			return sun_left ? push_shadows_from_left : push_shadows_from_right;
			}();

		for (auto y = std::int32_t{}; y < table_area.height; ++y)
		{
			const auto x_val = sun_left ? table_area.x : row_length + table_area.x - 1;
			calculate_lighting_tile_row({ x_val, y + table_area.y }, light_table,
				dir, sun_rise, tile_sizef, row_length, shared, shadow_func);
		}

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			// TODO: to_world_vector
			const auto position = vector2_float{
				float_cast(pos.x) * tile_sizef,
				float_cast(pos.y) * tile_sizef
			};

#ifdef HADES_TERRAIN_USE_SIMPLE_NORMALS
			const auto& light_info = light_table[pos];
			const auto normals = calc_simple_normal(light_info.tri_normals);
#elif defined(HADES_TERRAIN_USE_BLENDED_NORMALS)
			//const auto normals = calculate_vertex_normal(pos, light_table, map_size_tiles);
#endif
			const auto triangle_height = get_height_for_cell(pos, shared.map, *shared.settings);
			const auto type = pick_triangle_type(pos);
			const auto positions = make_cell_positions(position, tile_sizef);
			const auto quad = make_terrain_triangles(positions, light_info.height, type, {}, light_info.shadow_height, normals);
			
			shared.quads.append(quad);

			const auto adj_cliffs = get_adjacent_cliffs(pos, shared.map);
			const auto cliffs = make_cliffs(pos, position, light_info.height, adj_cliffs, tile_sizef, shared);
			for (auto i = rect_edges::begin; i != rect_edges::end; i = next(i))
			{
				const auto index = enum_type(i);
				if (!cliffs[index])
					continue;

				// if cliffs[index] is present, then cliff_normals must be aswell.
				assert(light_info.cliff_normals[index]);
				const auto cliff_normal = calc_simple_normal(*light_info.cliff_normals[index]);

				auto shdw = light_info.shadow_height;
				switch (i)
				{
				case rect_edges::top:
					shdw[enum_type(rect_corners::bottom_left)] = shdw[enum_type(rect_corners::top_left)];
					shdw[enum_type(rect_corners::bottom_right)] = shdw[enum_type(rect_corners::top_right)];
					break;
				case rect_edges::right:
					shdw[enum_type(rect_corners::top_left)] = shdw[enum_type(rect_corners::top_right)];
					shdw[enum_type(rect_corners::bottom_left)] = shdw[enum_type(rect_corners::bottom_right)];
					break;
				case rect_edges::bottom:
					shdw[enum_type(rect_corners::top_left)] = shdw[enum_type(rect_corners::bottom_left)];
					shdw[enum_type(rect_corners::top_right)] = shdw[enum_type(rect_corners::bottom_right)];
					break;
				case rect_edges::left:
					shdw[enum_type(rect_corners::top_right)] = shdw[enum_type(rect_corners::bottom_left)];
					shdw[enum_type(rect_corners::bottom_right)] = shdw[enum_type(rect_corners::top_left)];
					break;
				}

				const auto& c = *cliffs[index];
				const auto cliff_quad = make_terrain_triangles(c.positions, c.height, type, {}, shdw, cliff_normal);
				shared.quads.append(cliff_quad);
			}
			return;
		});

		return;
	}

	static void generate_edits(mutable_terrain_map::shared_data& shared)
	{
		assert(shared.settings->editor_terrain);
		
		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto map_size_tiles = get_size(shared.map);

		if (shared.edit_target_style == mutable_terrain_map::edit_target::tile)
		{
			const auto& edit_tiles = resources::get_transitions(*shared.settings->editor_terrain, resources::transition_tile_type::all, *shared.settings);
			assert(!empty(edit_tiles));
			const auto& edit_tile = edit_tiles.front();
			shared.edit_tex = edit_tile.tex.get();

			const auto tex_coords = rect_float{
				float_cast(edit_tile.left),
				float_cast(edit_tile.top),
				tile_sizef,
				tile_sizef
			};

			for (const auto& pos : shared.edit_targets)
			{
				const auto position = vector2_float{
					float_cast(pos.x) * tile_sizef,
					float_cast(pos.y) * tile_sizef
				};

				const auto triangle_height = get_height_for_cell(pos, shared.map, *shared.settings);
				const auto type = pick_triangle_type(pos);
				const auto p = make_cell_positions(position, tile_sizef);
				const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords);

				shared.quads.append(quad);
			}
		}
		else // shared.edit_target_style == mutable_terrain_map::edit_target::vertex
		{
			auto get_tile = [&](auto&& tile_type)->const resources::tile& {
				const auto& edit_tiles = resources::get_transitions(*shared.settings->editor_terrain, tile_type, *shared.settings);
				assert(!empty(edit_tiles));
				return edit_tiles.front();
				};

			const auto& top_left = get_tile(resources::transition_tile_type::top_left);
			const auto& top_right = get_tile(resources::transition_tile_type::top_right);
			const auto& bottom_left = get_tile(resources::transition_tile_type::bottom_left);
			const auto& bottom_right = get_tile(resources::transition_tile_type::bottom_right);
			shared.edit_tex = bottom_right.tex.get();

			for (const auto& pos : shared.edit_targets)
			{
				// make a graphic in each adjacent tile
				for_each_safe_adjacent_corner(shared.map, pos, [&](auto&& tile_pos, auto&& corner) {
					const auto triangle_height = get_height_for_cell(tile_pos, shared.map, *shared.settings);
					const auto type = pick_triangle_type(tile_pos);
					const auto position = vector2_float{
						float_cast(tile_pos.x) * tile_sizef,
						float_cast(tile_pos.y) * tile_sizef
					};
					const auto p = make_cell_positions(position, tile_sizef);

					resources::tile tile [[indeterminate]];

					switch (corner)
					{
					default:
						[[fallthrough]];
					case rect_corners::top_left:
						tile = top_left;
						break;
					case rect_corners::top_right:
						tile = top_right;
						break;
					case rect_corners::bottom_left:
						tile = bottom_left;
						break;
					case rect_corners::bottom_right:
						tile = bottom_right;
						break;
					}

					const auto tex_coords = rect_float{
						float_cast(tile.left),
						float_cast(tile.top),
						tile_sizef,
						tile_sizef
					};

					const auto quad = make_terrain_triangles(p, triangle_height, type, tex_coords);
					shared.quads.append(quad);
					return;
				});
			}
		}

		return;
	}

	struct chunk_flags
	{
		bool shadows : 1;
		bool grid : 1;
		bool cliffs : 1;
		bool layers : 1;
		bool ramps : 1;
	};

	static void generate_chunk(mutable_terrain_map::shared_data& shared,
		std::vector<map_tile>& tile_buffer,	const rect_int terrain_area,
		const chunk_flags flags)
	{
		//generate terrain layers
		const auto s = size(shared.map.terrain_layers);
		for (auto i = s; i > 0; --i)
			generate_layer(shared, tile_buffer, shared.map.terrain_layers[i - 1], terrain_area);

		generate_cliffs(shared, tile_buffer, terrain_area);
		
		shared.start_lighting = shared.quads.size();
		if (flags.shadows)
			generate_lighting(shared, terrain_area);

		shared.start_grid = shared.quads.size();
		if (flags.grid)
			generate_grid(shared, terrain_area);

		shared.start_ramp = shared.quads.size();
		if (flags.ramps)
			generate_ramp(shared, terrain_area);

		shared.start_cliff_debug = shared.quads.size();
		if (flags.cliffs)
			generate_cliff_debug(shared, terrain_area);

		if (flags.layers)
			generate_cliff_layer_debug(shared, terrain_area);

		return;
	}

	void mutable_terrain_map::apply()
	{
		if (!_needs_update)
			return;

		_shared.regions.clear();
		_shared.quads.clear();

		const auto flags = chunk_flags{ _show_shadows, _show_grid, _show_cliff_edges, _show_cliff_layers, _show_ramps };

		const auto tile_sizef = float_cast(_shared.settings->tile_size);
		auto tiled_start = static_cast<vector2_int>(position(_shared.world_area) / tile_sizef);
		tiled_start.x = std::max(0, tiled_start.x);
		tiled_start.y = std::max(0, tiled_start.y);

		const auto tiled_size = static_cast<vector2_int>(size(_shared.world_area) / tile_sizef);

		// working buffer to sort tiles in
		// TODO: store this in the terrain_map so we don't have to alloc every map update
		auto tile_buffer = std::vector<hades::map_tile>{};
		
		// TODO: clamp tile_start/area to regions in the game world
		generate_chunk(_shared, tile_buffer, { tiled_start, tiled_size }, flags);

		// generate map editor targets
		// TODO: we might have to clear out the previous editor targets if they've changed
		_shared.start_edit = _shared.quads.size();
		generate_edits(_shared);

		_shared.quads.apply();
		_needs_update = false;
		return;
	}

	void mutable_terrain_map::draw(sf::RenderTarget& t, const sf::RenderStates& states) const
	{
		assert(!_needs_update);

		auto s = states;
		s.shader = _debug_depth ? _shader_debug_depth.get_shader() : _shader.get_shader();
		
		glEnable(GL_CULL_FACE);
		depth_buffer::setup();
		depth_buffer::clear();
		depth_buffer::enable();

		for (const auto& region : _shared.regions)
		{
			assert(size(_shared.texture_table) > region.texture_index);
			s.texture = &resources::texture_functions::get_sf_texture(_shared.texture_table[region.texture_index].get());
			_shared.quads.draw(t, region.start_index, region.end_index - region.start_index, s);
		}

		//shadows
		if (_show_shadows)
		{
			const auto pop_shader = s.shader;
			s.shader = _debug_shadows ? _shader_debug_lighting.get_shader() : _shader_shadows_lighting.get_shader();
			_shared.quads.draw(t, _shared.start_lighting, _shared.start_grid - _shared.start_lighting, s);
			s.shader = pop_shader;
		}

		if (_show_grid && _shared.grid_tex)
		{
			s.texture = &resources::texture_functions::get_sf_texture(_shared.grid_tex);
			_shared.quads.draw(t, _shared.start_grid, _shared.start_ramp - _shared.start_grid, s);
		}

		if (_show_ramps && _shared.ramp_tex)
		{
			s.texture = &resources::texture_functions::get_sf_texture(_shared.ramp_tex);
			_shared.quads.draw(t, _shared.start_ramp, _shared.start_cliff_debug - _shared.start_ramp, s);
		}

		if (_show_cliff_edges && _shared.cliff_debug_tex)
		{
			s.texture = &resources::texture_functions::get_sf_texture(_shared.cliff_debug_tex);
			_shared.quads.draw(t, _shared.start_ramp, _shared.start_edit - _shared.start_cliff_debug, s);
		}

		glDisable(GL_CULL_FACE);

		if (_show_cliff_layers && _shared.layer_tex)
		{
			s.texture = _shared.layer_tex;
			t.draw(_shared.cliff_layer_debug, s);
		}

		s.texture = &resources::texture_functions::get_sf_texture(_shared.edit_tex);
		_shared.quads.draw(t, _shared.start_edit, _shared.quads.size() - _shared.start_edit, s);
		depth_buffer::disable();
		return;
	}

	void mutable_terrain_map::set_world_rotation(const float rot)
	{
		auto t = sf::Transform{};
		t.rotate(sf::degrees(rot));
		auto& l = _shared.local_bounds;
		auto t_rect = t.transformRect(sf::FloatRect{ {l.x, l.y}, {l.width, l.height} });

		auto y_offset = t_rect.top;
		auto y_range = t_rect.height - y_offset;

		auto matrix = sf::Glsl::Mat4{ t };

		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting, &_shader_debug_lighting })
		{
			shdr->set_uniform("rotation_matrix"sv, matrix);
			shdr->set_uniform("y_offset"sv, y_offset);
			shdr->set_uniform("y_range"sv, y_range);
		}
		return;
	}

	void mutable_terrain_map::set_sun_angle(float degrees)
	{
		if (degrees == 0.f)
			degrees = 0.01f;

		degrees = std::clamp(degrees, 0.f, 180.f);
		_shared.sun_angle_radians = to_radians(degrees);
		_needs_update = true;

		constexpr auto half_pi = std::numbers::pi_v<float> / 2.f;
		const auto z_angle = remap(to_radians(degrees), 0.f, std::numbers::pi_v<float>, half_pi, -half_pi);
		const auto sun_unit_vect = to_vector(basic_pol_vector<float, 3>{ 0.f, z_angle, 1.f });
		_shader_shadows_lighting.set_uniform("sun_direction"sv, sun_unit_vect);
		return;
	}

	void mutable_terrain_map::place_terrain(const terrain_vertex_position p, const resources::terrain* t)
	{
		hades::place_terrain(_shared.map, p, t, *_shared.settings);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::raise_terrain(const terrain_vertex_position v, const terrain_map::vertex_height_t amount)
	{
		change_terrain_height(v, _shared.map, *_shared.settings, detail::add_height_functor{ amount });
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::lower_terrain(const terrain_vertex_position v, const terrain_map::vertex_height_t amount)
	{
		change_terrain_height(v, _shared.map, *_shared.settings, detail::sub_height_functor{ amount });
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::set_terrain_height(const terrain_vertex_position v, const terrain_map::vertex_height_t h)
	{
		change_terrain_height(v, _shared.map, *_shared.settings, detail::set_height_functor{ h });
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::raise_cliff(const tile_position p)
	{
		change_terrain_cliff_layer(p, _shared.map, *_shared.settings, detail::add_height_functor{ 1 });
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::lower_cliff(const tile_position p)
	{
		change_terrain_cliff_layer(p, _shared.map, *_shared.settings, detail::sub_height_functor{ 1 });
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::set_cliff(const tile_position p, const terrain_map::cliff_layer_t layer)
	{
		change_terrain_cliff_layer(p, _shared.map, *_shared.settings, detail::set_height_functor{ layer });
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::place_ramp(const tile_position p)
	{
		_needs_update |= hades::place_ramp(p, _shared.map, *_shared.settings);
	}

	void mutable_terrain_map::remove_ramp(const tile_position p)
	{
		_needs_update |= clear_ramp(p, _shared.map, *_shared.settings);
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

	void terrain_mini_map::update(const terrain_map&)
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
