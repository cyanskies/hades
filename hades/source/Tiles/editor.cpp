#include "Tiles/editor.hpp"

#include "Hades/common-input.hpp"
#include "Hades/Data.hpp"

namespace tiles
{
	void tile_editor::init()
	{
		if (_tileInfo.texture == hades::UniqueId::Zero)
		{
			//no tile to fill the map with
			const auto tile_setting_id = hades::data::GetUid(resources::tile_settings_name);
			TileSettings = hades::data::Get<resources::tile_settings>(tile_setting_id);

			_tileInfo = GetErrorTile();
		}	

		object_editor::init();
	}

	void tile_editor::reinit()
	{
		object_editor::reinit();
	}

	void tile_editor::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		target.setView(GameView);
		DrawBackground(target);
		target.draw(Map);
		DrawGrid(target);
		DrawObjects(target);

		DrawPreview(target);
	}

	void tile_editor::FillToolBar(AddToggleButtonFunc tog, AddButtonFunc but, AddSeperatorFunc sep)
	{
		object_editor::FillToolBar(tog, but, sep);
	}
	void tile_editor::FillGui()
	{
		object_editor::FillGui();
	}

	void tile_editor::GenerateDrawPreview(const sf::RenderTarget& window, MousePos m)
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
	void tile_editor::OnClick(const sf::RenderTarget &t, MousePos m)
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

	void tile_editor::NewLevelDialog()
	{
		object_editor::NewLevelDialog();
	}

	void tile_editor::NewLevel()
	{
		object_editor::NewLevel();

		const auto settings = GetTileSettings();

		const tile_count_t width = MapSize.x / settings->tile_size;
		TileArray map{ width * MapSize.y / settings->tile_size, _tileInfo };
		Map.create({ map, width });
	}

	void tile_editor::SaveLevel() const
	{}

	void tile_editor::LoadLevel()
	{}

	void tile_editor::SaveTiles()
	{}

	void tile_editor::LoadTiles()
	{}

	void tile_editor::DrawPreview(sf::RenderTarget& target) const
	{
		if (EditMode == editor::EditMode::TILE)
			target.draw(_tilePreview);
		else
			object_editor::DrawPreview(target);
	}
}
