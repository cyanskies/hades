#include "OrthoTerrain/editor.hpp"

#include "SFGUI/Widgets.hpp"

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

		auto palette = GetPaletteContainer();

		palette->RemoveAll();

		_terrainWindow = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 1.f);

		palette->PackEnd(_terrainWindow);

		_addTerrain(_terrainset->terrains);
	}

	void terrain_editor::_addTerrain(const std::vector<const resources::terrain*> &terrain)
	{
		assert(_terrainWindow);
		_terrainWindow->RemoveAll();

		const auto palette_alloc = GetPaletteContainer()->GetRequisition();
		auto box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
		_terrainWindow->PackEnd(box, false, false);

		using objects::editor::CreateButton;

		for (const auto terr : terrain)
		{
			//get a full terrain tile to use as the image
			const auto &full_tiles = terr->full;
			if (full_tiles.empty())
			{
				//LOG error
				//TODO:
				continue;
			}

			const auto t = full_tiles.front();

			//create an animation to pass to 'CreateButton'
			hades::resources::animation a;
			a.tex = t.texture;
			a.height = a.width = TileSettings->tile_size;
			hades::resources::animation_frame f{ t.left, t.top, 1.f };
			a.value.push_back(f);

			auto b = objects::editor::CreateButton("unavailable", [this, terr]
			{
				_setCurrentTerrain(terr);
			}, &a);

			const auto alloc = box->GetRequisition();
			const auto button_alloc = b->GetAllocation();

			if (alloc.x + button_alloc.width > palette_alloc.x)
			{
				box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
				_terrainWindow->PackEnd(box, false);
			}

			box->PackEnd(b, false);
		}
	}

	void terrain_editor::_setCurrentTerrain(const resources::terrain*)
	{

	}
}