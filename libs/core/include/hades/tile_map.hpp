#ifndef HADES_TILE_MAP_HPP
#define HADES_TILE_MAP_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/exceptions.hpp"
#include "hades/level.hpp"
#include "hades/texture.hpp"
#include "hades/tiles.hpp"
#include "hades/types.hpp"
#include "hades/vertex_buffer.hpp"

//adds support for drawing tile maps
//TODO: split the draw into smaller buffers to better support giant maps

namespace hades
{
	//registers the resources needed to use tilemaps
	//same as the one in tiles.hpp but also registers the texture loading
	void register_tile_map_resources(data::data_manager&);

	//thrown by tile maps for unrecoverable errors
	class tile_map_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	//an immutable type for drawing tile_map instances
	//this is much more performant that the variant that supports changing the map
	class immutable_tile_map : public sf::Drawable
	{
	public:
		immutable_tile_map() noexcept = default;
		immutable_tile_map(const immutable_tile_map&) = default;
		immutable_tile_map(const tile_map&);

		virtual ~immutable_tile_map() {}

		virtual void create(const tile_map&);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		rect_float get_local_bounds() const;

	protected:
		struct texture_layer
		{
			const resources::texture *texture = nullptr;
			quad_buffer quads;
		};

		void create(const tile_map&, sf::VertexBuffer::Usage);

		std::vector<texture_layer> texture_layers;
		rect_float local_bounds{};
	};

	//an expanded upon immutable_tile_map, that allows changing the map on the fly
	//mostly used for editors
	class mutable_tile_map final : public immutable_tile_map
	{
	public:
		mutable_tile_map() = default;
		mutable_tile_map(const mutable_tile_map&) = default;
		mutable_tile_map(const tile_map&);

		mutable_tile_map &operator=(const mutable_tile_map&) = default;

		void create(const tile_map&) override;
		void update(const tile_map&);

		//replaces the tiles in the listed positions with tile&
		//use the helpers in tiles.hpp to create position vectors in various shapes
		void place_tile(const std::vector<tile_position>&, const resources::tile&);
		
		tile_map get_map() const;

	private:
		void _apply();
		void _update_tile(tile_position, const resources::tile&);
		void _replace_tile(std::size_t layer, std::size_t, tile_position, resources::tile_size_t, const resources::tile&);
		void _remove_tile(std::size_t layer, std::size_t);
		void _remove_layer(std::size_t layer);
		void _add_tile(std::size_t layer, tile_position, resources::tile_size_t, const resources::tile&);

		std::vector<bool> _dirty_buffers;
		tile_map _tiles;
	};
}

#endif // !HADES_TILES_HPP
