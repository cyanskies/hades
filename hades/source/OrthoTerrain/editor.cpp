#include "OrthoTerrain/editor.hpp"

#include "SFGUI/Widgets.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/files.hpp"

namespace ortho_terrain
{
	void terrain_editor::init()
	{
		if (resources::TerrainSets.empty())
		{
			LOGERROR("No Terrainsets have been recorded, editor unavailable");
			kill();
			return;
		}
		
		const auto terrainset_id = resources::TerrainSets.front();
		const auto terrainset = hades::data::Get<resources::terrainset>(terrainset_id);

		Terrainset(terrainset);

		tile_editor::init();
	}

	void terrain_editor::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		target.setView(GameView);
		DrawBackground(target);
		DrawTerrain(target);
		target.draw(Map);
		DrawGrid(target);
		DrawObjects(target);
		DrawPreview(target);
	}

	void terrain_editor::FillToolBar(AddToggleButtonFunc toggle, AddButtonFunc button, AddSeperatorFunc seperator)
	{
		tile_editor::FillToolBar(toggle, button, seperator);

		seperator();

		button("terrain", [this] {
			_enterTerrainMode();
		}, nullptr);

		button("erase terrain", [this] {
			_enterTerrainMode();
			static const auto empty_terrain = hades::data::Get<resources::terrain>(resources::EmptyTerrainId);
			_setCurrentTerrain(empty_terrain);
		}, nullptr);
	}

	void terrain_editor::GenerateDrawPreview(const sf::RenderTarget &t, MousePos m)
	{
		if (Mode() == editor::EditMode::TERRAIN
			&& _terrainMode == editor::TerrainEditMode::TERRAIN)
		{
			const auto tile_size = TileSettings->tile_size;

			const auto [x, y] = m;

			auto truePos = t.mapPixelToCoords({ x, y }, GameView);
			truePos += {static_cast<float>(tile_size), static_cast<float>(tile_size)};

			auto vertPos = static_cast<sf::Vector2i>(
				sf::Vector2f{ std::round(truePos.x / tile_size), std::round(truePos.y / tile_size) });

			if (_drawPosition != vertPos)
			{
				_drawPosition = vertPos;
				_preview = _map;
				_preview.setColour(sf::Color::Transparent);
				_preview.replace(_terrain, _drawPosition, GetDrawSize());
			}
		}

		tile_editor::GenerateDrawPreview(t, m);
	}

	void terrain_editor::OnModeChange(EditMode_t t)
	{
		if (t != editor::EditMode::TERRAIN)
			_terrainWindow = nullptr;

		tile_editor::OnModeChange(t);
	}

	void terrain_editor::OnClick(const sf::RenderTarget &t, MousePos m)
	{
		if (Mode() == editor::EditMode::TERRAIN
			&& _terrainMode == editor::TerrainEditMode::TERRAIN)
		{
			//place the tile in the tile map

			_map.replace(_terrain, _drawPosition, GetDrawSize());
		}
		else
			tile_editor::OnClick(t, m);
	}

	void terrain_editor::NewLevelDialog()
	{
		//TODO:
		tile_editor::NewLevelDialog();
	}

	void terrain_editor::NewLevel()
	{
		tile_editor::NewLevel();

		const auto width = MapSize.x / TileSettings->tile_size;
		const auto height = MapSize.y / TileSettings->tile_size;
		_map.create(_terrainset->terrains, width, height);
	}

	void terrain_editor::SaveLevel() const
	{
		level l;
		SaveObjects(l);
		SaveTiles(l);
		SaveTerrain(l);

		YAML::Emitter e;
		e << YAML::BeginMap;
		objects::WriteObjectsToYaml(l, e);
		tiles::WriteTilesToYaml(l, e);
		WriteTerrainToYaml(l, e);
		e << YAML::EndMap;

		const auto path = Mod + '/' + Filename;
		hades::files::write_file(path, e.c_str());
	}

	void terrain_editor::LoadLevel()
	{
		auto level_str = hades::files::as_string(Mod, Filename);
		auto level_yaml = YAML::Load(level_str);

		level lvl;
		objects::ReadObjectsFromYaml(level_yaml, lvl);
		ReadTilesFromYaml(level_yaml, lvl);
		ReadTerrainFromYaml(level_yaml, lvl);

		LoadObjects(lvl);
		LoadTiles(lvl);
		LoadTerrain(lvl);

		reinit();
	}

	void terrain_editor::SaveTerrain(level &l) const
	{
		const auto map = _map.getMap();
		l.terrain = as_terrain_layer(map, MapSize.x / TileSettings->tile_size);
	}

	void terrain_editor::LoadTerrain(const level &l)
	{
		const auto width = MapSize.x / TileSettings->tile_size;
		const auto map = as_terraindata(l.terrain, width);
		_map.create(map, width);
	}

	void terrain_editor::DrawPreview(sf::RenderTarget &target) const
	{
		if (Mode() == editor::EditMode::TERRAIN
			&& _terrainMode == editor::TerrainEditMode::TERRAIN)
		{
			target.draw(_preview);
		}
		else
			tile_editor::DrawPreview(target);
	}

	void terrain_editor::DrawTerrain(sf::RenderTarget &target)
	{
		target.draw(_map);
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

	void terrain_editor::_setCurrentTerrain(const resources::terrain *t)
	{
		_terrain = t;
		_terrainMode = editor::TerrainEditMode::TERRAIN;
	}
}