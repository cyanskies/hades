#include "Tiles/editor.hpp"

#include "Hades/common-input.hpp"

namespace tiles
{
	template<class MapClass>
	void tile_editor_t<MapClass>::init()
	{
		const auto tile_setting_id = hades::data::GetUid(resources::tile_settings_name);
		TileSettings = hades::data::Get<resources::tile_settings>(tile_setting_id);

		if (_tileInfo.texture == hades::UniqueId::Zero)
		{
			//replace FillTile with an error tile
		}

		object_editor::init();
	}

	template<class MapClass>
	void tile_editor_t<MapClass>::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		target.setView(GameView);
		DrawBackground(target);
		target.draw(Map);
		DrawGrid(target);
		DrawObjects(target);

		DrawPreview(target);
	}

	template<class MapClass>
	void tile_editor_t<MapClass>::GenerateDrawPreview(const sf::RenderTarget& window, MousePos m)
	{
		assert(TileSettings);
		const auto mousePos = m;
		if (EditMode == editor::EditMode::TILE)
		{
			const auto tile_size = TileSettings->tile_size;

			const auto x = std::get<0>(mousePos);
			const auto y = std::get<1>(mousePos);

			auto truePos = window.mapPixelToCoords({ x, y }, GameView);
			truePos += {static_cast<float>(tile_size), static_cast<float>(tile_size)};

			auto snapPos = truePos - sf::Vector2f(static_cast<float>(std::abs(std::fmod(truePos.x, tile_size))),
				static_cast<float>(std::abs((std::fmod(truePos.y, tile_size)))));
			auto position = sf::Vector2i(snapPos) / static_cast<int>(tile_size);
			if (_tilePosition != position)
			{
				_tilePosition = position;
				_tilePreview = Map;
				_tilePreview.replace(_tileInfo, _tilePosition, TileDrawSize);
			}
		}
		else
			object_editor::GenerateDrawPreview(window, m);
	}

	//pastes the currently selected tile or terrain onto the map
	template<class MapClass>
	void tile_editor_t<MapClass>::OnClick(const sf::RenderTarget &t, MousePos m)
	{
		const auto mouseLeft = m;
		if (EditMode == editor::EditMode::TILE)
		{
			//place the tile in the tile map
			Map.replace(_tileInfo, _tilePosition, TileDrawSize);
		}
		else
			object_editor::OnClick(t, m);
	}

	template<class MapClass>
	void tile_editor_t<MapClass>::DrawPreview(sf::RenderTarget& target) const
	{
		if (EditMode == editor::EditMode::TILE)
			target.draw(_tilePreview);
		else
			object_editor::DrawPreview(target);
	}
}
