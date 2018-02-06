#include "OrthoTerrain/editor.hpp"

#include <thread> //see: FillTileList()
				  // async building of tile selector ui

//#include "TGUI/Animation.hpp"
#include "TGUI/Widgets/HorizontalWrap.hpp"
#include "TGUI/Widgets/EditBox.hpp"
#include "TGUI/Widgets/Picture.hpp"

#include "Hades/common-input.hpp"
#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"
//#include "Hades/files.hpp"
//#include "Hades/Properties.hpp"
//#include "Hades/StandardPaths.hpp"

#include "OrthoTerrain/generator.hpp"
#include "OrthoTerrain/resources.hpp"

#include "Tiles/resources.hpp"
#include "Tiles/tiles.hpp"

//map size in tiles
const hades::types::uint32 map_height = 100, map_width = 100;

namespace ortho_terrain
{
	void terrain_editor::generate()
	{
		auto error_terrain = GetErrorTerrain();

		auto t_id = hades::data::UniqueId::Zero;

		for (auto t : resources::Terrains)
		{
			if (t != error_terrain)
			{
				t_id = t;
				break;
			}
		}

		//TODO: handle lack of terrains
		auto terrain = hades::data::Get<resources::terrain>(t_id);
		const auto map = generator::Blank({ map_height, map_width }, terrain);
		Map.create(map);
	}

	void terrain_editor::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		target.setView(GameView);
		target.draw(Map);

		if (EditMode == editor::TERRAIN)
			target.draw(_terrainPreview);
		else
			DrawTilePreview(target);
	}

	void terrain_editor::GenerateDrawPreview(const sf::RenderTarget& window, const hades::InputSystem::action_set &input)
	{
		auto mousePos = input.find(hades::input::PointerPosition);

		if (EditMode == tiles::editor::EditMode::TILE)
			tiles::tile_editor_t<MutableTerrainMap>::GenerateDrawPreview(window, input);
		else if (mousePos != input.end() && mousePos->active)
		{
			const auto tile_size = TileSettings->tile_size;

			auto truePos = window.mapPixelToCoords({ mousePos->x_axis, mousePos->y_axis }, GameView);
			truePos += {static_cast<float>(tile_size), static_cast<float>(tile_size)};

			if (EditMode == editor::EditMode::TERRAIN)
			{
				auto vertPos = static_cast<sf::Vector2u>(
					sf::Vector2f{ std::round(truePos.x / tile_size), std::round(truePos.y / tile_size) });
				if (vertPos == _terrainPosition)
					return;

				_terrainPosition = vertPos;

				_terrainPreview = Map;
				assert(_terrainInfo);
				_terrainPreview.replace(*_terrainInfo, vertPos, _terrain_draw_size);
			}
		}
	}

	//pastes the currently selected tile or terrain onto the map
	void terrain_editor::TryDraw(const hades::InputSystem::action_set &input)
	{
		tiles::tile_editor_t<MutableTerrainMap>::TryDraw(input);

		auto mouseLeft = input.find(hades::input::PointerLeft);
		if (mouseLeft != input.end() && mouseLeft->active)
		{
			if (EditMode == editor::EditMode::TERRAIN)
			{
				Map.replace(*_terrainInfo, _terrainPosition, _terrain_draw_size);
			}
		}
	}

	void terrain_editor::FillTileSelector()
	{
		tiles::tile_editor_t<MutableTerrainMap>::FillTileSelector();

		const auto settings = tiles::GetTileSettings();
		auto tile_size = settings.tile_size;
		auto tile_button_size = tile_size * 3;

		 //================================
		 //fill the terrain selector window
		 //================================

		static const auto terrain_size = "terrain-size";
		auto terrainSize = _gui.get<tgui::EditBox>(terrain_size);
		if (terrainSize)
		{
			terrainSize->onTextChange.connect([this, terrainSize]() {
				try
				{
					auto value = std::stoi(terrainSize->getText().toAnsiString());
					if (value >= 0)
						_terrain_draw_size = value;
				}
				//throws invalid_argument and out_of_range, 
				//we can't do anything about either of these
				//in this case we just leave the terrain edit size as it was
				catch (...)
				{
					return;
				}
			});
		}

		static const auto terrain_container = editor::terrain_selector_panel;
		auto terrainContainer = _gui.get<tgui::Container>(terrain_container);

		if (!terrainContainer)
		{
			LOGERROR("terrain_editor Gui failed, missing container for 'terrain'");
			kill();
			return;
		}

		auto terrainLayout = tgui::HorizontalWrap::create();
		auto terrains = resources::Terrains;

		std::thread terrain_work([this, terrainLayout, terrainContainer, terrains, settings, tile_button_size]() {
			//place available terrain in the "terrain" container
			for (auto t : terrains)
			{
				if (hades::data::Exists(t) && t != GetErrorTerrain())
				{
					auto terrain = hades::data::Get<resources::terrain>(t);
					assert(terrain);

					if (terrain->tiles.empty())
						continue;

					auto tile = terrain->tiles[0];

					//skip if needed data is missing.
					if (!hades::data::Exists(tile.texture))
					{
						continue;
						LOGERROR("terrain_editor UI skipping tile because texture is missing: " + hades::data::GetAsString(tile.texture));
					}

					auto texture = hades::data::Get<hades::resources::texture>(terrain->tiles[0].texture);
					auto tex = tgui::Texture(texture->value, { static_cast<int>(tile.left), static_cast<int>(tile.top),
						static_cast<int>(settings.tile_size), static_cast<int>(settings.tile_size) });

					auto tileButton = tgui::Picture::create(tex);
					tileButton->setSize(tile_button_size, tile_button_size);

					tileButton->onClick.connect([this, terrain]() {
						EditMode = editor::EditMode::TERRAIN;
						_terrainInfo = terrain;
					});

					terrainLayout->add(tileButton);
				}//if exists

			}//for Terrains
			terrainContainer->add(terrainLayout);
		});

		terrain_work.detach();
	}
}