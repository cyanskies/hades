#include "Tiles/editor.hpp"

namespace tiles
{
	template<class MapClass>
	void tile_editor_t<MapClass>::load_map(const MapData& data)
	{
		Map.create(data);
	}

	template<class MapClass>
	MapData tile_editor_t<MapClass>::save_map() const
	{
		return Map.getMap();
	}
	
	template<class MapClass>
	void tile_editor_t<MapClass>::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		target.setView(GameView);
		target.draw(Map);
		
		DrawTilePreview(target);
	}

	template<class MapClass>
	sf::FloatRect tile_editor_t<MapClass>::GetMapBounds() const
	{
		return Map.getLocalBounds();
	}

	template<class MapClass>
	void tile_editor_t<MapClass>::GenerateDrawPreview(const sf::RenderTarget& window, const hades::InputSystem::action_set &input)
	{
		auto mousePos = input.find(hades::input::PointerPosition);
		if (EditMode == editor::EditMode::TILE && mousePos != input.end() && mousePos->active)
		{
			const auto tile_size = TileSettings->tile_size;

			auto truePos = window.mapPixelToCoords({ mousePos->x_axis, mousePos->y_axis }, GameView);
			truePos += {static_cast<float>(tile_size), static_cast<float>(tile_size)};

			auto snapPos = truePos - sf::Vector2f(static_cast<float>(std::abs(std::fmod(truePos.x, tile_size))),
				static_cast<float>(std::abs((std::fmod(truePos.y, tile_size)))));
			auto position = sf::Vector2u(snapPos) / tile_size;
			if (_tilePosition != position)
			{
				_tilePosition = position;
				_tilePreview = Map;
				_tilePreview.replace(TileInfo, _tilePosition, Tile_draw_size);
			}
		}
	}

	//pastes the currently selected tile or terrain onto the map
	template<class MapClass>
	void tile_editor_t<MapClass>::TryDraw(const hades::InputSystem::action_set &input)
	{
		auto mouseLeft = input.find(hades::input::PointerLeft);
		if (EditMode == editor::EditMode::TILE && mouseLeft != input.end() && mouseLeft->active)
		{
			//place the tile in the tile map
			Map.replace(TileInfo, _tilePosition, Tile_draw_size);
		}
	}

	template<class MapClass>
	void tile_editor_t<MapClass>::DrawTilePreview(sf::RenderTarget& target) const
	{
		if (EditMode == editor::EditMode::TILE)
			target.draw(_tilePreview);
	}
}