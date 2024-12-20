#ifndef HADES_TERRAIN_MAP_HPP
#define HADES_TERRAIN_MAP_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "hades/gui.hpp"
#include "hades/terrain.hpp"
#include "hades/tile_map.hpp"

namespace hades
{
	void register_terrain_map_resources(data::data_manager&);

	class mutable_terrain_map final : public sf::Drawable
	{
	public:
		mutable_terrain_map();
		// NOTE: mutable_terrain_map is non-copyable, we supply this
		// to enable its storage in std::any; invoking this always throws logic_error
		mutable_terrain_map(const mutable_terrain_map&); 
		mutable_terrain_map(mutable_terrain_map&&) noexcept = default;
		mutable_terrain_map(terrain_map);

		mutable_terrain_map& operator=(mutable_terrain_map&) = delete;
		mutable_terrain_map& operator=(mutable_terrain_map&&) noexcept = default;

		void reset(terrain_map); // equiv to creating a new object of this class, except we retain our allocated memory

		// changed viewed area
		void set_world_region(rect_float) noexcept;

		// forces a realloc of all current chunks
		void set_chunk_size(std::size_t);

		void set_height_enabled(bool b) noexcept;

		void draw_depth_buffer(bool b) noexcept
		{
			_debug_depth = b;
			return;
		}

		bool draw_depth_buffer() const noexcept
		{
			return _debug_depth;
		}

		void draw_normals_buffer(bool b) noexcept
		{
			_debug_shadows = b;
			return;
		}

		bool draw_normals_buffer() const noexcept
		{
			return _debug_shadows;
		}

		void show_shadows(bool b) noexcept
		{
			_show_shadows = b;
			for (auto& chunk : _shared.chunks)
				chunk.needs_update = std::min(chunk.needs_update, chunk_data::update_flags::lighting);
			return;
		}

		bool show_shadows() const noexcept
		{
			return _show_shadows;
		}

		void show_grid(bool b) noexcept
		{
			_show_grid = b;
			for (auto& chunk : _shared.chunks)
				chunk.needs_update = std::min(chunk.needs_update, chunk_data::update_flags::grid);
			return;
		}

		bool show_grid() const noexcept
		{
			return _show_grid;
		}

		void show_ramps(bool b) noexcept
		{
			_show_ramps = b;
			for (auto& chunk : _shared.chunks)
				chunk.needs_update = std::min(chunk.needs_update, chunk_data::update_flags::ramp);
			return;
		}

		bool show_ramps() const noexcept
		{
			return _show_ramps;
		}

		void show_cliff_edges(bool b) noexcept
		{
			_show_cliff_edges = b;
			for (auto& chunk : _shared.chunks)
				chunk.needs_update = std::min(chunk.needs_update, chunk_data::update_flags::cliff_edge);
			return;
		}

		bool show_cliff_edges() const noexcept
		{
			return _show_cliff_edges;
		}

		void show_cliff_layers(bool b) noexcept
		{
			_show_cliff_layers = b;
			for (auto& chunk : _shared.chunks)
				chunk.cliff_layer_update = true;
			return;
		}

		bool show_cliff_layers() const noexcept
		{
			return _show_cliff_layers;
		}

		// TODO: regions

		void set_world_rotation(float degrees);
		void set_sun_angle(float degrees);

		void place_terrain(terrain_vertex_position, const resources::terrain*);

		// basic height editing
		void raise_terrain(terrain_vertex_position, terrain_map::vertex_height_t amount);
		void lower_terrain(terrain_vertex_position, terrain_map::vertex_height_t amount);
		void set_terrain_height(terrain_vertex_position, terrain_map::vertex_height_t h);
		
		void raise_cliff(tile_position);
		void lower_cliff(tile_position);
		void set_cliff(tile_position, terrain_map::cliff_layer_t);

		void place_ramp(tile_position);
		void remove_ramp(tile_position);

		enum class edit_target : bool 
		{
			vertex,
			tile
		};

		void clear_edit_target() noexcept
		{
			_shared.edit_targets.clear();
			_needs_update = true;
		}

		void set_edit_target_style(const edit_target style) noexcept
		{
			_shared.edit_target_style = style;
			_needs_update = true;
		}

		void add_edit_target(tile_position p)
		{
			_shared.edit_targets.emplace_back(p);
			_needs_update = true;
		}

		void apply();
		void draw(sf::RenderTarget& target, const sf::RenderStates& states) const override;

		rect_float get_local_bounds() const noexcept
		{
			return _shared.local_bounds;
		}

		const terrain_map& get_map() const noexcept
		{
			return _shared.map;
		}

		bool is_height_enabled() const noexcept
		{
			return _show_height;
		}

		std::size_t chunk_count() const noexcept
		{
			return std::size(_shared.chunks);
		}

	public:
		struct vertex_region
		{
			// TODO: redo these with smaller sizes
			std::size_t start_index;
			std::size_t end_index;
			std::uint8_t texture_index;
		};

		struct chunk_data
		{
			enum class update_flags : std::uint8_t {
				all, // vertex regions
				lighting,
				grid,
				ramp,
				cliff_edge,
				cliff_value,
				end
			};

			quad_buffer quads;
			// terrain and cliff data
			std::vector<vertex_region> regions;
			rect_int tile_bounds = {};
			std::size_t start_lighting;
			std::size_t start_grid;
			std::size_t start_ramp;
			std::size_t start_cliff_debug;
			sf::VertexBuffer cliff_layer_debug = { sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Static };
			update_flags needs_update;
			bool cliff_layer_update;
		};

		struct shared_data
		{
			terrain_map map;
			quad_buffer quads;
			std::vector<tile_position> edit_targets;
			gui gui = { "" }; // Don't generate .ini
			std::vector<chunk_data> chunks;
			std::vector<resources::resource_link<resources::texture>> texture_table;
			const resources::terrain_settings* settings = {};
			// Debug textures
			const resources::texture* grid_tex = {}; // grid overlay
			const resources::texture* ramp_tex = {}; // ramp location overlay
			const resources::texture* cliff_debug_tex = {}; // cliff edge overlay
			const sf::Texture* layer_tex = {}; // cliff height overlay
			// editor selection texture (and region display texture)
			const resources::texture* edit_tex = {};
			// total local bounds of the terrain_map
			rect_float local_bounds = {};
			std::size_t start_edit; // start index for editor selection
			float sun_angle_radians = {};
			edit_target edit_target_style = edit_target::vertex;
		};

	private:
		// token that defaults to true when copied or moved
		// TODO: make terrain_map non-copyable and we won't need this
		class boolean_token
		{
		public:
			boolean_token() noexcept = default;
			boolean_token(bool b) noexcept
				: _val{ b }
			{}

			boolean_token(const boolean_token&) noexcept
				: _val{ true }
			{}

			boolean_token(boolean_token&&) noexcept
				: _val{ true }
			{}

			boolean_token& operator=(const boolean_token&) noexcept
			{
				_val = true;
				return *this;
			}

			boolean_token& operator=(bool b) noexcept
			{
				_val = b;
				return *this;
			}

			boolean_token& operator|=(bool b) noexcept
			{
				_val |= b;
				return *this;
			}

			boolean_token& operator=(boolean_token&&) noexcept
			{
				_val = true;
				return *this;
			}

			operator bool() const noexcept
			{
				return _val;
			}

		private:
			bool _val = true;
		};

		resources::shader_proxy _shader; // turns red channel into height
		resources::shader_proxy _shader_debug_depth; // as above; draw depth buffer
		// red channel into height, green channel into shadow height
		// blue and alpha channel store normals
		resources::shader_proxy _shader_shadows_lighting;
		resources::shader_proxy _shader_debug_lighting;

	private:
		shared_data _shared;
		// currently viewed area
		rect_float _view_area = {};
		std::size_t _chunk_size = std::numeric_limits<std::size_t>::max();
		boolean_token _needs_update = true; // edits and regions
		bool _show_grid : 1 = false;
		bool _show_height : 1 = true;
		bool _show_shadows : 1 = true;
		bool _debug_depth : 1 = false;
		bool _debug_shadows : 1 = false;
		bool _show_ramps : 1 = false;
		bool _show_cliff_edges : 1 = false;
		bool _show_cliff_layers : 1 = false;
		bool _show_regions : 1 = false;
	};

	constexpr auto moveable = std::is_nothrow_move_assignable_v< mutable_terrain_map>;

	class terrain_mini_map final : public sf::Drawable, public sf::Transformable
	{
	public:
		terrain_mini_map() noexcept = default;

		void set_size(const vector2_float s);
		void update(const terrain_map&);

	protected:
		void draw(sf::RenderTarget&, const sf::RenderStates&) const override;

	private:
		vector2_float _size;
		quad_buffer  _quad;
		std::unique_ptr<sf::Texture> _texture;
	};

}

#endif // !HADES_TERRAIN_MAP_HPP
