#ifndef HADES_TERRAIN_MAP_HPP
#define HADES_TERRAIN_MAP_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "hades/terrain.hpp"
#include "hades/tile_map.hpp"

namespace hades
{
	void register_terrain_map_resources(data::data_manager&);

	class immutable_terrain_map  final : public sf::Drawable
	{
	public:
		immutable_terrain_map() = default;
		immutable_terrain_map(const immutable_terrain_map&) = default;
		immutable_terrain_map(const terrain_map&);

		void create(const terrain_map&);

		void draw(sf::RenderTarget& target, const sf::RenderStates& states) const override;

		rect_float get_local_bounds() const noexcept;

	private:
		immutable_tile_map _tile_layer;
		std::vector<immutable_tile_map> _terrain_layers;
	};

	class mutable_terrain_map final : public sf::Drawable
	{
	public:
		mutable_terrain_map();
		mutable_terrain_map(const mutable_terrain_map&) = default;
		mutable_terrain_map(terrain_map);

		void reset(terrain_map); // equiv to creating a new object of this class, except we retain our allocated memory

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

		void show_shadows(bool b) noexcept
		{
			_show_shadows = b;
			return;
		}

		bool show_shadows() const noexcept
		{
			return _show_shadows;
		}

		void show_grid(bool b) noexcept
		{
			_show_grid = b;
			return;
		}

		bool show_grid() const noexcept
		{
			return _show_grid;
		}

		void set_world_rotation(float degrees);
		void set_sun_angle(float degrees);

		void place_tile(tile_position, const resources::tile&);
		void place_terrain(terrain_vertex_position, const resources::terrain*);

		void raise_terrain(terrain_vertex_position, uint8_t amount);
		void lower_terrain(terrain_vertex_position, uint8_t amount);

		//void create_cliff(terrain_vertex_position, terrain_vertex_position)

		void replace_heightmap(const std::vector<std::uint8_t>& height) noexcept
		{
			assert(size(height) == size(_map.heightmap));
			_map.heightmap = height;
			return;
		}

		void apply();
		void draw(sf::RenderTarget& target, const sf::RenderStates& states) const override;

		rect_float get_local_bounds() const noexcept
		{
			return _local_bounds;
		}

		const terrain_map& get_map() const noexcept
		{
			return _map;
		}

		bool is_height_enabled() const noexcept
		{
			return _show_height;
		}

	public:
		// internal structures
		struct map_tile
		{
			tile_index_t index;
			std::array<std::uint8_t, 4> height = {};
			resources::tile_size_t left;
			resources::tile_size_t top;
			std::uint8_t texture = {};
			std::uint8_t layer = {};
		};

		struct shadow_tile
		{
			bool operator<(const shadow_tile& rhs) const noexcept
			{
				return index < rhs.index;
			}

			tile_index_t index;
			std::array<std::uint8_t, 4> height = {};
		};

		struct map_info
		{
			std::vector<map_tile> tile_info;
			std::vector<resources::resource_link<resources::texture>> texture_table;
		};

	private:
		class auto_texture_ptr
		{
		public:
			auto_texture_ptr() noexcept = default;
			auto_texture_ptr(std::unique_ptr<sf::Texture> t) noexcept
				: _ptr{ std::move(t) }
			{}

			auto_texture_ptr(const auto_texture_ptr& p)
				: _ptr{ p._ptr ? std::make_unique<sf::Texture>(*p._ptr) : std::unique_ptr<sf::Texture>{} }
			{}
			auto_texture_ptr(auto_texture_ptr&&) noexcept = default;

			auto_texture_ptr& operator=(const auto_texture_ptr& p)
			{
				_ptr = p._ptr ? std::make_unique<sf::Texture>(*p._ptr) : std::unique_ptr<sf::Texture>{};
				return *this;
			}
			auto_texture_ptr& operator=(auto_texture_ptr&&) noexcept = default;

			sf::Texture* operator->() noexcept
			{
				return _ptr.get();
			}

			sf::Texture& operator*() noexcept
			{
				return *_ptr;
			}

			operator bool() const noexcept
			{
				return _ptr.get();
			}

		private:
			std::unique_ptr<sf::Texture> _ptr;
		};

		// token that defaults to true when copied or moved
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

		map_info _info;
		resources::shader_proxy _shader;
		resources::shader_proxy _shader_debug_depth;
		resources::shader_proxy _shader_shadows_lighting;
		rect_float _local_bounds = {};
		const resources::terrain_settings* _settings = {};
		const resources::texture* _grid_tex = {};
		std::size_t _shadow_start = {};
		std::size_t _grid_start = {};
		std::set<shadow_tile> _shadow_tile_info;
		auto_texture_ptr _sunlight_table;
		float _sun_angle = to_radians(135.f);
		bool _show_grid = false;
		boolean_token _needs_apply = false;
		bool _show_height = true;
		bool _show_shadows = true;
		bool _debug_depth = false;

	private:
		//std::array<bool, 4> _chunks;
		terrain_map _map;
		quad_buffer _quads;
	};

	class mutable_terrain_map2 final : public sf::Drawable
	{
	public:
		mutable_terrain_map2();
		mutable_terrain_map2(const mutable_terrain_map2&) = default;
		mutable_terrain_map2(terrain_map);

		void reset(terrain_map); // equiv to creating a new object of this class, except we retain our allocated memory

		void set_world_region(rect_float) noexcept;

		//void set_chunk_size(std::size_t) noexcept;

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

		void show_shadows(bool b) noexcept
		{
			_show_shadows = b;
			return;
		}

		bool show_shadows() const noexcept
		{
			return _show_shadows;
		}

		void show_grid(bool b) noexcept
		{
			_show_grid = b;
			return;
		}

		bool show_grid() const noexcept
		{
			return _show_grid;
		}

		void set_world_rotation(float degrees);
		void set_sun_angle(float degrees);

		void place_tile(tile_position, const resources::tile&);
		void place_terrain(terrain_vertex_position, const resources::terrain*);

		void raise_terrain(terrain_vertex_position, uint8_t amount);
		void lower_terrain(terrain_vertex_position, uint8_t amount);

		//void create_cliff(terrain_vertex_position, terrain_vertex_position)

		void replace_heightmap(const std::vector<std::uint8_t>& height) noexcept
		{
			assert(size(height) == size(_shared.map.heightmap));
			_shared.map.heightmap = height;
			_needs_update = true;
			return;
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

	public:
		// internal structures
		struct map_tile
		{
			tile_index_t index;
			std::array<std::uint8_t, 4> height = {};
			resources::tile_size_t left;
			resources::tile_size_t top;
			std::uint8_t texture = {};
			std::uint8_t layer = {};
		};

		struct shadow_tile
		{
			bool operator<(const shadow_tile& rhs) const noexcept
			{
				return index < rhs.index;
			}

			tile_index_t index;
			std::array<std::uint8_t, 4> height = {};
		};

		struct map_info
		{
			std::vector<map_tile> tile_info;
		};

		struct vertex_region
		{
			// redo these with smaller sizes
			std::size_t start_index;
			std::size_t end_index;
			std::uint8_t texture_index;
		};

		struct shared_data
		{
			rect_float world_area = {};
			terrain_map map;
			quad_buffer quads;
			const resources::terrain_settings* settings = {};
			// per chunk data
			std::vector<vertex_region> regions;
			std::size_t start_lighting;
			std::size_t start_grid;
			// end per chunk data
			std::vector<resources::resource_link<resources::texture>> texture_table;
			const resources::texture* grid_tex = {};
			rect_float local_bounds = {};
			float sun_angle_radians = to_radians(135.f);
		};

	private:
		class auto_texture_ptr
		{
		public:
			auto_texture_ptr() noexcept = default;
			auto_texture_ptr(std::unique_ptr<sf::Texture> t) noexcept
				: _ptr{ std::move(t) }
			{}

			auto_texture_ptr(const auto_texture_ptr& p)
				: _ptr{ p._ptr ? std::make_unique<sf::Texture>(*p._ptr) : std::unique_ptr<sf::Texture>{} }
			{}
			auto_texture_ptr(auto_texture_ptr&&) noexcept = default;

			auto_texture_ptr& operator=(const auto_texture_ptr& p)
			{
				_ptr = p._ptr ? std::make_unique<sf::Texture>(*p._ptr) : std::unique_ptr<sf::Texture>{};
				return *this;
			}
			auto_texture_ptr& operator=(auto_texture_ptr&&) noexcept = default;

			sf::Texture* operator->() noexcept
			{
				return _ptr.get();
			}

			sf::Texture& operator*() noexcept
			{
				return *_ptr;
			}

			operator bool() const noexcept
			{
				return _ptr.get();
			}

		private:
			std::unique_ptr<sf::Texture> _ptr;
		};

		// token that defaults to true when copied or moved
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

		map_info _info;
		resources::shader_proxy _shader;
		resources::shader_proxy _shader_debug_depth;
		resources::shader_proxy _shader_shadows_lighting;
		//std::size_t _shadow_start = {};
		//std::size_t _grid_start = {};
		std::set<shadow_tile> _shadow_tile_info;
		auto_texture_ptr _sunlight_table;

	private:
		/*struct chunk_info
		{
			std::vector<vertex_region> regions;
			rect_float area;
			bool active = false;
			bool needs_update;
		};*/

		//std::size_t _chunk_size = std::numeric_limits<std::size_t>::max(); // redo this with a smaller size
		//std::array<chunk_info, 4> _chunks;
		shared_data _shared;
		boolean_token _needs_update = true;
		bool _show_grid = false;
		bool _show_height = true;
		bool _show_shadows = true;
		bool _debug_depth = false;
	};

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
