#include "Tiles/editor.hpp"

#include "SFGUI/Widgets.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/files.hpp"
#include "Hades/simple_resources.hpp"

namespace tiles
{
	tile GetEmptyTile(const resources::tile_settings *s)
	{
		const auto tileset = s->empty_tileset;
		assert(!tileset->tiles.empty());
		return tileset->tiles[0];
	}

	void tile_editor::init()
	{
		TileSettings = hades::data::Get<resources::tile_settings>(id::tile_settings);	

		_tileInfo = GetEmptyTile(TileSettings);

		_tilePreview.setColour(sf::Color::Transparent);

		object_editor::init();
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

	void tile_editor::FillToolBar(AddToggleButtonFunc toggle, AddButtonFunc button, AddSeperatorFunc seperator)
	{
		object_editor::FillToolBar(toggle, button, seperator);

		seperator();

		button("tiles", [this] {
			_enterTileMode();
		}, nullptr);

		button("erase tile", [this] {
			_enterTileMode();
			_setCurrentTile(GetEmptyTile(TileSettings));
		}, nullptr);

		button("draw size-", [this] {
			_tileDrawSize = std::max(1, _tileDrawSize - 1);
		}, nullptr);

		button("draw size+", [this] {
			_tileDrawSize = std::min(10, _tileDrawSize + 1);
		}, nullptr);
	}

	void tile_editor::GenerateDrawPreview(const sf::RenderTarget& window, MousePos m)
	{
		assert(TileSettings);
		const auto mousePos = m;
		if (Mode() == editor::EditMode::TILE)
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
				_tilePreview.setColour(sf::Color::Transparent);
				_tilePreview.replace(_tileInfo, _tilePosition, _tileDrawSize);
			}
		}
		else
			object_editor::GenerateDrawPreview(window, m);
	}

	void tile_editor::OnModeChange(EditMode_t t)
	{
		if(t != editor::EditMode::TILE)
			_tileWindow = nullptr;

		object_editor::OnModeChange(t);
	}

	//pastes the currently selected tile or terrain onto the map
	void tile_editor::OnClick(const sf::RenderTarget &t, MousePos m)
	{
		if (Mode() == editor::EditMode::TILE)
		{
			//place the tile in the tile map
			Map.replace(_tileInfo, _tilePosition, _tileDrawSize);
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

		if (!_tileInfo.texture)
		{
			//no tile to fill the map with
			_tileInfo = GetErrorTile();
		}

		assert(MapSize.x % TileSettings->tile_size == 0);

		const tile_count_t width = MapSize.x / TileSettings->tile_size;
		TileArray map{ width * MapSize.y / TileSettings->tile_size, _tileInfo };
		Map.create({ map, width });
	}

	void tile_editor::SaveLevel() const
	{
		level l;
		SaveObjects(l);
		SaveTiles(l);

		YAML::Emitter e;
		e << YAML::BeginMap;
		objects::WriteObjectsToYaml(l, e);
		WriteTilesToYaml(l, e);
		e << YAML::EndMap;

		const auto path = Mod + '/' + Filename;
		hades::files::write_file(path, e.c_str());
	}

	void tile_editor::LoadLevel()
	{
		auto level_str = hades::files::as_string(Mod, Filename);
		auto level_yaml = YAML::Load(level_str);

		level lvl;
		objects::ReadObjectsFromYaml(level_yaml, lvl);
		ReadTilesFromYaml(level_yaml, lvl);

		LoadObjects(lvl);
		LoadTiles(lvl);

		reinit();
	}

	void tile_editor::SaveTiles(level &l) const
	{
		const auto map_data = Map.getMap();
		const auto map = as_rawmap(map_data);

		using tileset_list = std::vector<TileSetInfo>;
		const auto &tilesets = std::get<tileset_list>(map);
		for (const auto &tset : tilesets)
		{
			const auto id = std::get<hades::UniqueId>(tset);
			const auto name = hades::data::GetAsString(id);
			const auto first_tid = std::get<tile_count_t>(tset);
			l.tilesets.push_back({ name, first_tid });
		}

		using tile_array = std::vector<tile_count_t>;
		const auto &tiles = std::get<tile_array>(map);
		l.tiles = tiles;

		//level editor shouldn't be able to produce an invalid map width
		using map_width_t = tile_count_t;
		const auto width = std::get<map_width_t>(map);
		assert(l.map_x % TileSettings->tile_size == 0);
		assert(width == l.map_x / TileSettings->tile_size);
	}

	void tile_editor::LoadTiles(const level &l)
	{
		if (l.map_x % TileSettings->tile_size != 0)
			throw objects::level_load_exception{ "Unable to load tile layer, tiles cannot fit in the provided map size" };

		const auto width = l.map_x / TileSettings->tile_size;

		std::vector<TileSetInfo> tilesets;
		for (const auto &tset : l.tilesets)
		{
			const auto name = std::get<hades::types::string>(tset);
			const auto id = hades::data::GetUid(name);
			const auto first_tid = std::get<tile_count_t>(tset);

			tilesets.push_back({ id, first_tid });
		}

		const auto map = as_mapdata({ tilesets, l.tiles, width });
		Map.create(map);
	}

	void tile_editor::DrawPreview(sf::RenderTarget& target) const
	{
		if (Mode() == editor::EditMode::TILE)
			target.draw(_tilePreview);
		else
			object_editor::DrawPreview(target);
	}

	void tile_editor::Tilesets(std::vector<const resources::tileset*> tilesets)
	{
		std::swap(_tilesets, tilesets);
	}

	draw_size_t tile_editor::GetDrawSize() const
	{
		return _tileDrawSize;
	}

	void tile_editor::_enterTileMode()
	{
		Mode(editor::EditMode::TILE);
		_tileMode = editor::TileEditMode::NONE;
		_addTilesToUi();
	}

	void tile_editor::_addTilesToUi()
	{
		assert(Mode() == editor::EditMode::TILE);

		auto palette = GetPaletteContainer();

		palette->RemoveAll();

		auto combobox = sfg::ComboBox::Create();

		//add the selector combobox
		palette->PackEnd(combobox, false);

		_tileWindow = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 1.f);

		palette->PackEnd(_tileWindow);

		constexpr auto all_str = "all";

		combobox->AppendItem(all_str);

		const auto &tilesets = _tilesets;

		//add all the group names
		for (const auto &t : tilesets)
		{
			const auto tset = hades::data::GetAsString(t->id);
			combobox->AppendItem(tset);
		}

		std::weak_ptr<sfg::ComboBox> combo_weak{ combobox };
		auto on_select_item = [this, &tilesets, combo_weak] {
			const auto combo = combo_weak.lock();
			//selected is in the range [0 - groups.size() + 1]
			//[0] is all_string, everything else is groups[selected - 1]
			const auto selected = combo->GetSelectedItem();

			if (selected == 0)
			{
				std::vector<tile> tiles;

				for (const auto &t : tilesets)
					std::copy(std::begin(t->tiles), std::end(t->tiles), std::back_inserter(tiles));

				_addTiles(tiles);
			}
			else
			{
				assert(static_cast<int>(tilesets.size()) >= selected);
				const auto t = tilesets[selected - 1];
				_addTiles(t->tiles);
			}
		};

		combobox->GetSignal(sfg::ComboBox::OnSelect).Connect(on_select_item);

		combobox->SelectItem(0);
		on_select_item();
	}

	void tile_editor::_addTiles(const std::vector<tile> &tiles)
	{
		//replace the contents of _objectPalette with the objects in objects
		assert(_tileWindow);
		_tileWindow->RemoveAll();

		const auto palette_alloc = GetPaletteContainer()->GetRequisition();
		auto box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
		_tileWindow->PackEnd(box, false, false);

		using objects::editor::CreateButton;

		for (const auto t : tiles)
		{
			//create an animation to pass to 'CreateButton'
			hades::resources::animation a;
			a.tex = t.texture;
			a.height = a.width = TileSettings->tile_size;
			hades::resources::animation_frame f{ t.left, t.top, 1.f };
			a.value.push_back(f);

			auto b = objects::editor::CreateButton("unavailable", [this, t]
			{
				_setCurrentTile(t);
			}, &a);

			const auto alloc = box->GetRequisition();
			const auto button_alloc = b->GetAllocation();

			if (alloc.x + button_alloc.width > palette_alloc.x)
			{
				box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
				_tileWindow->PackEnd(box, false);
			}

			box->PackEnd(b, false);
		}
	}

	void tile_editor::_setCurrentTile(tile t)
	{
		_tileMode = editor::TileEditMode::TILE;
		_tileInfo = t;
	}
}
