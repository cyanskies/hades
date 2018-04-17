#include "OrthoTerrain/editor.hpp"

#include "Hades/Data.hpp"

namespace ortho_terrain
{
	//TODO: reuse tile draw size, the eraser tool from tiles

	void terrain_editor::init()
	{
		const auto terrainset_id = resources::TerrainSets.front();
		const auto terrainset = hades::data::Get<resources::terrainset>(terrainset_id);

		Terrainset(terrainset);
		
		tile_editor::init();
	}

	void terrain_editor::FillToolBar(AddToggleButtonFunc toggle, AddButtonFunc button, AddSeperatorFunc seperator)
	{
		tile_editor::FillToolBar(toggle, button, seperator);

		seperator();

		button("terrain", [this] {
			_enterTerrainMode();
		}, nullptr);
	}

	void terrain_editor::Terrainset(const resources::terrainset *t)
	{
		_terrainset = t;

		const auto terrains = t->terrains;
		//terrains can be used as tilesets, we give these to the tile_editor
		std::vector<const tiles::resources::tileset*> tilesets;
		std::copy(std::begin(terrains), std::end(terrains), std::back_inserter(tilesets));

		tile_editor::Tilesets(tilesets);
	}

	void terrain_editor::_enterTerrainMode()
	{
		Mode(editor::EditMode::TERRAIN);
		_terrainMode = editor::TerrainEditMode::NONE;
		_addTerrainToGui();
	}

	void terrain_editor::_addTerrainToGui()
	{
		assert(Mode() == editor::EditMode::TERRAIN);

	}
}