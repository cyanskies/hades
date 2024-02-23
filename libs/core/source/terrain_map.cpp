#include "hades/terrain_map.hpp"

#include <numbers>

#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/OpenGL.hpp"

#include "hades/draw_tools.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/sf_color.hpp"
#include "hades/shader.hpp"
#include "hades/table.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

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

constexpr auto fragment_normal_color = "gl_FragColor = texture2D(tex, gl_TexCoord[0].xy);";
constexpr auto fragment_depth_color = "gl_FragColor = vec4(gl_FragDepth, gl_FragDepth, gl_FragDepth, 1.f);tex;";

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
{
	// calculate fragment depth along y axis
	gl_FragDepth = 1.f - ((model_view_vertex_y - y_offset) / y_range);
	// shadow info
	float shadow_height = gl_Color.g;
	float frag_height = gl_Color.r;
	// lighting info
	float theta = gl_Color.b * 2 * pi;
	float phi = gl_Color.a * pi;
	float sin_phi = sin(phi);

	// interpolation can denomalise the origional normal
	vec3 denormal = vec3(cos(theta) * sin_phi, sin(theta) * sin_phi, cos(phi));
	vec3 normal = normalize(denormal);

	float cos_angle = clamp(dot(normal, sun_direction), 0.f, 1.f);
	vec4 light_col = vec4(mix(vec3(0.f, 0.f, 0.f), sun_colour.rgb, cos_angle), sun_colour.a);
	vec4 shadow_col = mix(vec4(0.f, 0.f, 0.f, 0.f) , shadow_col, float(frag_height < shadow_height) * 1.f);
	// frag colour
	gl_FragColor = mix(light_col, shadow_col, float(frag_height < shadow_height));
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
			shdr_funcs::set_vertex(*shader, vertex_source);
			shdr_funcs::set_uniforms(*shader, uniforms); // copy uniforms(we move it later)
		}

		if (terrain_map_shader_debug_id == unique_zero)
		{ // make heightmap debug terrain shader
			terrain_map_shader_debug_id = make_unique_id();

			auto shader = shdr_funcs::find_or_create(d, terrain_map_shader_debug_id);
			assert(shader);

			shdr_funcs::set_fragment(*shader, std::format(fragment_source, fragment_depth_color));
			shdr_funcs::set_vertex(*shader, vertex_source);
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
				},{
					"sun_direction"s,
					resources::uniform{
						resources::uniform_type_list::vector3f,
						{} // (0.f, 0.f, 0.f)
					}
				}
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
			shdr_funcs::set_vertex(*shader, vertex_source);
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
		_shader_debug_depth{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_debug_id)) },
		_shader_shadows_lighting{ shdr_funcs::get_shader_proxy(*shdr_funcs::get_resource(terrain_map_shader_shadows_lighting)) }
	{
		_shared.settings = resources::get_terrain_settings();
	}

	mutable_terrain_map::mutable_terrain_map(terrain_map t) : mutable_terrain_map{}
	{
		reset(std::move(t));
	}

	void mutable_terrain_map::reset(terrain_map t)
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

	/*void mutable_terrain_map::set_chunk_size(std::size_t s) noexcept
	{
		const auto d = std::div(s, _settings->tile_size);
		const auto new_size = d.quot + d.rem;
		if (_chunk_size != new_size)
			_chunk_size = new_size;

		return;
	}*/

	void mutable_terrain_map::set_height_enabled(bool b) noexcept
	{
		_show_height = b;
		for (auto shdr : { &_shader, &_shader_debug_depth, &_shader_shadows_lighting })
			shdr->set_uniform("height_multiplier"sv, _show_height * 1.f);
		return;
	}

	static poly_quad make_terrain_triangles(vector2_float pos, float tile_size,
		const triangle_height_data& h, rect_float texture_quad,
		const std::array<std::uint8_t, 4> &shadow_h = {}, 
		const std::array<std::array<std::uint8_t, 2>, 4> &normals = {}) noexcept
	{
		const auto quad = rect_float{ pos, { tile_size, tile_size } };
		const auto quad_right_x = quad.x + quad.width;
		const auto quad_bottom_y = quad.y + quad.height;
		const auto tex_right_x = texture_quad.x + texture_quad.width;
		const auto tex_bottom_y = texture_quad.y + texture_quad.height;

		auto sf_col = std::array<sf::Color, 6>{};
		for (auto i = std::uint8_t{}; i < 6; ++i)
		{
			const auto& normal = normals[triangle_index_to_quad_index(i, h.triangle_type)];
			sf_col[i] = sf::Color{
				h.height[i],
				shadow_h[triangle_index_to_quad_index(i, h.triangle_type)],
				normal[0], // theta
				normal[1]};// phi
		}
		
		if (h.triangle_type == terrain_map::triangle_uphill)
		{
			// uphill triangulation, first triangle edges against the left and top sides of the quad
			//						second triangle edges against the right and bottom sides of the quad
			return poly_quad{
				//first triangle
				sf::Vertex{ {quad.x, quad.y},					sf_col[0], { texture_quad.x, texture_quad.y } },//top left
				sf::Vertex{ { quad_right_x, quad.y },			sf_col[1], { tex_right_x, texture_quad.y } },	//top right
				sf::Vertex{ { quad.x, quad_bottom_y },			sf_col[2], { texture_quad.x, tex_bottom_y } },	//bottom left
				//second triangle
				sf::Vertex{ { quad_right_x, quad.y },			sf_col[3], { tex_right_x, texture_quad.y } },	//top right
				sf::Vertex{ { quad_right_x, quad_bottom_y },	sf_col[4], { tex_right_x, tex_bottom_y } },		//bottom right
				sf::Vertex{ { quad.x, quad_bottom_y },			sf_col[5], { texture_quad.x, tex_bottom_y } }	//bottom left
			};
		}
		else
		{
			// uphill triangulation, first triangle edges against the left and bottom sides of the quad
			//						second triangle edges against the right and top sides of the quad
			return poly_quad{
				//first triangle
				sf::Vertex{ {quad.x, quad.y},					sf_col[0], { texture_quad.x, texture_quad.y } },//top left
				sf::Vertex{ { quad_right_x, quad_bottom_y },	sf_col[1], { tex_right_x, tex_bottom_y } },		//bottom right
				sf::Vertex{ { quad.x, quad_bottom_y },			sf_col[2], { texture_quad.x, tex_bottom_y } },	//bottom left
				//second triangle
				sf::Vertex{ {quad.x, quad.y},					sf_col[3], { texture_quad.x, texture_quad.y } },//top left
				sf::Vertex{ { quad_right_x, quad.y },			sf_col[4], { tex_right_x, texture_quad.y } },	//top right
				sf::Vertex{ { quad_right_x, quad_bottom_y },	sf_col[5], { tex_right_x, tex_bottom_y } },		//bottom right
			};
		}
	}

	struct map_tile
	{
		tile_position position;
		//std::array<std::uint8_t, 4> height = {};
		resources::tile_size_t left;
		resources::tile_size_t top;
		std::uint8_t texture = {};
	};

	static void generate_layer(mutable_terrain_map::shared_data& shared,
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

			// add tile to the tile list
			tile_buffer.emplace_back(pos, tile.left, tile.top, integer_cast<std::uint8_t>(tex_index));
			return;
			});

		if (empty(tile_buffer))
			return;

		std::ranges::sort(tile_buffer, {}, &map_tile::texture);
		
		auto current_region = mutable_terrain_map::vertex_region{ 
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

			const auto triangle_height = get_height_for_triangles(tile.position, shared.map);
			const auto quad = make_terrain_triangles(pos, tile_sizef, triangle_height, tex_coords);
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

	static void generate_grid(mutable_terrain_map::shared_data& shared,
		const rect_int terrain_area)
	{
		assert(shared.settings->grid_terrain);
		const auto& grid_tiles = resources::get_transitions(*shared.settings->grid_terrain, resources::transition_tile_type::all, *shared.settings);
		assert(!empty(grid_tiles));
		const auto& grid_tile = grid_tiles.front();
		shared.grid_tex = grid_tile.tex.get();

		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto map_size_tiles = get_size(shared.map.tile_layer);

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

			const auto triangle_height = get_height_for_triangles(pos, shared.map);
			const auto quad = make_terrain_triangles(position, tile_sizef, triangle_height, tex_coords);

			shared.quads.append(quad);
			return;
			});
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
		const std::uint8_t l0 = current[1];
		const std::uint8_t l3 = current[2];
		const std::uint8_t l1 = l0 < sun_rise ? 0 : l0 - sun_rise;
		const std::uint8_t l2 = l3 < sun_rise ? 0 : l3 - sun_rise;

		const std::uint8_t n1 = next[0] < sun_rise ? 0 : next[0] - sun_rise;
		const std::uint8_t n2 = next[3] < sun_rise ? 0 : next[3] - sun_rise;

		return {
			std::max(l0, next[0]),
			std::max(l1, n1),
			std::max(l2, n2),
			std::max(l3, next[3])
		};
	}

	static constexpr std::array<std::uint8_t, 4> push_shadows_from_right(const std::array<std::uint8_t, 4>& current,
		const std::array<std::uint8_t, 4>& next, const std::uint8_t sun_rise) noexcept
	{
		const std::uint8_t l1 = current[0];
		const std::uint8_t l2 = current[3];
		const std::uint8_t l0 = l1 < sun_rise ? 0 : l1 - sun_rise;
		const std::uint8_t l3 = l2 < sun_rise ? 0 : l2 - sun_rise;

		const std::uint8_t n0 = next[1] < sun_rise ? 0 : next[1] - sun_rise;
		const std::uint8_t n3 = next[2] < sun_rise ? 0 : next[2] - sun_rise;

		return {
			std::max(l0, n0),
			std::max(l1, next[1]),
			std::max(l2, next[2]),
			std::max(l3, n3)
		};
	}

	struct lighting_info
	{
		struct normal {
			float theta,
				phi;
		};

		std::array<normal, 2> tri_normals;
		std::array<std::uint8_t, 4> height;
		std::array<std::uint8_t, 4> shadow_height;
		bool triangle_type;
	};

	template<typename Func>
	static void calculate_lighting_tile_row(tile_position p, table<lighting_info>&table, const tile_position dir,
		const std::uint8_t sun_rise, const terrain_map& m, const float tile_sizef,
		const std::uint32_t row_length, Func push_shadows) noexcept
	{
		constexpr auto top_left = enum_type(rect_corners::top_left),
			top_right = enum_type(rect_corners::top_right),
			bottom_right = enum_type(rect_corners::bottom_right),
			bottom_left = enum_type(rect_corners::bottom_left);

		const auto triangle_normal = [](const vector3<float> p1, const vector3<float> p2, const vector3<float> p3) constexpr noexcept -> vector3<float> {
			const auto u = p2 - p1;
			const auto v = p3 - p1;
			return vector::cross(u, v);
		};
		
		const auto max_val = [](auto a, auto b) {
			return std::max(a, b);
			};

		const auto shadow_tris = get_height_for_triangles(p, m);
		auto shadow_h = std::array{
			access_triangles_as_quad(shadow_tris.height, shadow_tris.triangle_type, 0, max_val),
			access_triangles_as_quad(shadow_tris.height, shadow_tris.triangle_type, 1, max_val),
			access_triangles_as_quad(shadow_tris.height, shadow_tris.triangle_type, 2, max_val),
			access_triangles_as_quad(shadow_tris.height, shadow_tris.triangle_type, 3, max_val),
		};

		for (auto i = std::uint32_t{}; i < row_length; ++i)
		{
			const auto h_tris = get_height_for_triangles(p, m);
			const auto h = std::array{
				access_triangles_as_quad(h_tris.height, h_tris.triangle_type, 0, max_val),
				access_triangles_as_quad(h_tris.height, h_tris.triangle_type, 1, max_val),
				access_triangles_as_quad(h_tris.height, h_tris.triangle_type, 2, max_val),
				access_triangles_as_quad(h_tris.height, h_tris.triangle_type, 3, max_val),
			};
			
			shadow_h = std::invoke(push_shadows, shadow_h, h, sun_rise);
			
			// calc triange normals
			// uphill triangles
			// TODO: select triangle type
			const auto posf = static_cast<vector2_float>(p) * tile_sizef;
			const auto normal_a = triangle_normal(
				{ posf.x, posf.y, float_cast(h[top_left]) },
				{ posf.x + tile_sizef, posf.y, float_cast(h[top_right]) },
				{ posf.x, posf.y + tile_sizef, float_cast(h[bottom_left]) });
			const auto normal_b = triangle_normal(
				{ posf.x + tile_sizef, posf.y, float_cast(h[top_right]) },
				{ posf.x + tile_sizef, posf.y + tile_sizef, float_cast(h[bottom_right]) },
				{ posf.x, posf.y + tile_sizef, float_cast(h[bottom_left]) });

			table[p] = lighting_info{ lighting_info::normal{ vector::angle_theta(normal_a), vector::angle_phi(normal_a) },
				lighting_info::normal{ vector::angle_theta(normal_b), vector::angle_phi(normal_b) },
				h, shadow_h, h_tris.triangle_type };

			p += dir;
		}
		return;
	}

	template<std::size_t N>
	static constexpr lighting_info::normal mean_vector(const std::array<lighting_info::normal, N> vects) noexcept
	{	
		const auto from_normal = [](const lighting_info::normal norm) constexpr noexcept -> vector3<float> {
			const auto pol = basic_pol_vector<float, 3>{ norm.theta, norm.phi, 1.f };
			return to_vector(pol);
			};
		const auto mean = vector::unit(std::transform_reduce(begin(vects), end(vects), vector3<float>{}, std::plus{}, from_normal));
		const auto pol = to_pol_vector(mean);

		constexpr auto pi = std::numbers::pi_v<float>;
		constexpr auto pi2 = std::numbers::pi_v<float> * 2.f;
		return { 
			normalise(pol.theta, 0.f, pi2), normalise(pol.phi, 0.f, pi)
		};
	}

	static std::array<std::array<std::uint8_t, 2>, 4> calculate_vertex_normal(const tile_position& p,
		const table<lighting_info>& table, const lighting_info& centre, const tile_position& world_size)
	{
		const auto to_uint_normal = [](const lighting_info::normal norm) constexpr noexcept -> std::array<std::uint8_t, 2> {
			constexpr auto pi = std::numbers::pi_v<float>;
			const auto theta = (norm.theta / (pi * 2.f)) * 255.f;
			const auto phi = (norm.phi / pi) * 255.f;
			return { integral_cast<std::uint8_t>(theta), integral_cast<std::uint8_t>(phi) };
		};

		const auto get_cell = [&](const tile_position& p) -> lighting_info {
			if (!within_world(p, world_size))
				return lighting_info{};

			return table[p];
		};

		// NOTE: tiles (x is 'centre'
		//			Top row:	0, 1, 2
		//			Middle row: 3, x, 4
		//			bottom row: 5, 6, 7
		const auto tiles = std::array{
			get_cell(p + tile_position{ -1, -1 }), // 0
			get_cell(p + tile_position{  0, -1 }), // 1
			get_cell(p + tile_position{  1, -1 }), // 2
			get_cell(p + tile_position{ -1,  0 }), // 3
			get_cell(p + tile_position{  1,  0 }), // 4
			get_cell(p + tile_position{ -1,  1 }), // 5
			get_cell(p + tile_position{  0,  1 }), // 6
			get_cell(p + tile_position{  1,  1 }), // 7
		};

		constexpr auto top_left = 0, top = 1, top_right = 2,
						left = 3, right = 4, 
						bottom_left = 5, bottom = 6, bottom_right = 7;

		// TODO: triangle type
		// average each of the contributing normals
		const auto v0 = mean_vector(std::array{ centre.tri_normals[0],
			tiles[top_left].tri_normals[1],
			tiles[top].tri_normals[0], tiles[top].tri_normals[1],
			tiles[left].tri_normals[0], tiles[left].tri_normals[1] });
		const auto v1 = mean_vector(std::array{ centre.tri_normals[0], centre.tri_normals[1],
			tiles[top].tri_normals[1], 
			tiles[top_right].tri_normals[0], tiles[top_right].tri_normals[1],
			tiles[right].tri_normals[0] });
		const auto v2 = mean_vector(std::array{ centre.tri_normals[1] ,
			tiles[right].tri_normals[0], tiles[right].tri_normals[1],
			tiles[bottom_right].tri_normals[0],
			tiles[bottom].tri_normals[0], tiles[bottom].tri_normals[1]  });
		const auto v3 = mean_vector(std::array{ centre.tri_normals[0], centre.tri_normals[1],
			tiles[bottom].tri_normals[0],
			tiles[bottom_left].tri_normals[0], tiles[bottom_left].tri_normals[1],
			tiles[left].tri_normals[1] });

		return { 
			to_uint_normal(v0),
			to_uint_normal(v1),
			to_uint_normal(v2),
			to_uint_normal(v3),
		};
	}

	static void generate_lighting(mutable_terrain_map::shared_data& shared,
		const rect_int terrain_area)
	{
		const auto& sun_angle_radian = shared.sun_angle_radians;
		const auto tile_sizef = float_cast(shared.settings->tile_size);
		const auto sun_unit_vect = to_vector(pol_vector2_t<float>{ sun_angle_radian, 1.f });
		const auto slope = sun_unit_vect.y / sun_unit_vect.x;
		const auto next_y = sun_unit_vect.y * slope * (tile_sizef - sun_unit_vect.x);

		// difference in height over x
		const auto sun_rise = integral_clamp_cast<std::uint8_t>(std::abs(next_y));
		// max x difference that could effect height
		const auto sun_dist = next_y == 0.f ? std::uint8_t{} : integral_clamp_cast<std::uint8_t>(255.f / std::abs(next_y));
		const auto sun_hidden = sun_angle_radian > std::numbers::pi_v<float> || sun_angle_radian < 0.f;
		const auto sun_90 = sun_angle_radian == std::numbers::pi_v<float> / 2.f;
		const auto sun_left = sun_angle_radian > std::numbers::pi_v<float> / 2.f;

		const auto dir = [](bool left) noexcept {
			if (!left)
				return tile_position{ -1, 0 };
			else
				return tile_position{ 1, 0 };
			}(sun_left);

		const auto map_size_tiles = get_size(shared.map.tile_layer);
		
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
			calculate_lighting_tile_row({ x_val, y + table_area.y }, light_table, dir, sun_rise, shared.map, tile_sizef, row_length, shadow_func);
		}

		for_each_safe_position_rect(position(terrain_area), size(terrain_area), map_size_tiles, [&](const tile_position pos) {
			const auto position = vector2_float{
				float_cast(pos.x) * tile_sizef,
				float_cast(pos.y) * tile_sizef
			};

			const auto& light_info = light_table[pos];
			const auto normals = calculate_vertex_normal(pos, light_table, light_info, map_size_tiles);

			const auto triangle_height = get_height_for_triangles(pos, shared.map);
			const auto quad = make_terrain_triangles(position, tile_sizef, triangle_height, {}, light_info.shadow_height, normals);

			shared.quads.append(quad);
			return;
		});

		return;
	}

	static void generate_chunk(mutable_terrain_map::shared_data& shared,
		std::vector<map_tile>& tile_buffer,	const rect_int terrain_area,
		const bool shadows, const bool grid)
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
			shared.start_lighting = shared.quads.size();
			if(shadows)
				generate_lighting(shared, terrain_area);

			shared.start_grid = shared.quads.size();
			if(grid)
				generate_grid(shared, terrain_area);
		}

		return;
	}

	void mutable_terrain_map::apply()
	{
		if (!_needs_update)
			return;

		_shared.regions.clear();
		_shared.quads.clear();

		const auto tiled_area = static_cast<vector2_int>(size(_shared.world_area) / float_cast(_shared.settings->tile_size));

		auto tile_buffer = std::vector<hades::map_tile>{};
		tile_buffer.reserve(integer_cast<std::size_t>(tiled_area.x) * integer_cast<std::size_t>(tiled_area.y));

		const auto tiled_start = static_cast<vector2_int>(position(_shared.world_area) / float_cast(_shared.settings->tile_size));

		generate_chunk(_shared, tile_buffer, { tiled_start, tiled_area }, _show_shadows, _show_grid);

		_shared.quads.apply();
		_needs_update = false;
		return;
	}

	void mutable_terrain_map::draw(sf::RenderTarget& t, const sf::RenderStates& states) const
	{
		assert(!_needs_update);

		auto s = states;
		s.shader = _debug_depth ? _shader_debug_depth.get_shader() : _shader.get_shader();
		
		depth_buffer::setup();
		depth_buffer::clear();
		depth_buffer::enable();

		for (const auto& region : _shared.regions)
		{
			assert(size(_shared.texture_table) > region.texture_index);
			s.texture = &resources::texture_functions::get_sf_texture(_shared.texture_table[region.texture_index].get());
			_shared.quads.draw(t, region.start_index, region.end_index - region.start_index, s);
		}

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

	void mutable_terrain_map::set_world_rotation(const float rot)
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

	void mutable_terrain_map::place_tile(const tile_position p, const resources::tile& t)
	{
		hades::place_tile(_shared.map, p, t, *_shared.settings);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::place_terrain(const terrain_vertex_position p, const resources::terrain* t)
	{
		hades::place_terrain(_shared.map, p, t, *_shared.settings);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::raise_terrain(const terrain_vertex_position p, const uint8_t amount)
	{
		hades::raise_terrain(_shared.map, p, amount, _shared.settings);
		_needs_update = true;
		return;
	}

	void mutable_terrain_map::lower_terrain(const terrain_vertex_position p, const uint8_t amount)
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
