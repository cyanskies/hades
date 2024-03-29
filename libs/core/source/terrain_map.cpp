#include "hades/terrain_map.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

namespace hades
{
	void register_terrain_map_resources(data::data_manager &d)
	{
		register_texture_resource(d);
		register_terrain_resources(d, resources::texture_functions::make_resource_link);
	}

	immutable_terrain_map::immutable_terrain_map(const terrain_map &t) :
		_tile_layer{t.tile_layer}
	{
		for (const auto &l : t.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void immutable_terrain_map::create(const terrain_map &t)
	{
		_tile_layer.create(t.tile_layer);
		_terrain_layers.clear();
		for (const auto &l : t.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void immutable_terrain_map::draw(sf::RenderTarget &t, const sf::RenderStates& s) const
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

	mutable_terrain_map::mutable_terrain_map(terrain_map t) :
		_map{ std::move(t) }, _tile_layer{ _map.tile_layer }
	{
		for (const auto &l : _map.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void mutable_terrain_map::create(terrain_map t)
	{
		_map = std::move(t);
		_tile_layer.create(_map.tile_layer);
		_terrain_layers.clear();
		for (const auto &l : _map.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void mutable_terrain_map::update(terrain_map t)
	{
		_map = std::move(t);
		_tile_layer.update(_map.tile_layer);

		assert(std::size(_map.terrain_layers) == std::size(_terrain_layers));

		auto iter = std::begin(_map.terrain_layers);
		auto iter2 = std::begin(_terrain_layers);
        for (/*iter, iter2*/; iter != std::end(_map.terrain_layers); ++iter, ++iter2)
			iter2->update(*iter);
	}

	void mutable_terrain_map::draw(sf::RenderTarget &t, const sf::RenderStates& s) const
	{
		for (auto iter = std::rbegin(_terrain_layers);
			iter != std::rend(_terrain_layers); ++iter)
			t.draw(*iter, s);

		t.draw(_tile_layer, s);
	}

	rect_float mutable_terrain_map::get_local_bounds() const noexcept
	{
		return _tile_layer.get_local_bounds();
	}

	void mutable_terrain_map::place_tile(const std::vector<tile_position> &p, const resources::tile &t)
	{
		hades::place_tile(_map, p, t);
		_tile_layer.place_tile(p, t);
	}

	void mutable_terrain_map::place_terrain(const std::vector<terrain_vertex_position> &p, const resources::terrain *t)
	{
		hades::place_terrain(_map, p, t);
		
		_tile_layer.update(_map.tile_layer);

		auto iter = std::begin(_terrain_layers);
		auto iter2 = std::begin(_map.terrain_layers);
        for (/*iter, iter2*/; iter != std::end(_terrain_layers); ++iter, ++iter2)
			iter->update(*iter2);
	}

	const terrain_map &mutable_terrain_map::get_map() const noexcept
	{
		return _map;
	}
}
