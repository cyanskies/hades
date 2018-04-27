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

	constexpr auto dialog_style = sfg::Window::Style::BACKGROUND
		| sfg::Window::Style::TITLEBAR
		| sfg::Window::Style::CLOSE
		| sfg::Window::Style::SHADOW
		| sfg::Window::Style::RESIZE;

	void terrain_editor::NewLevelDialog()
	{
		auto window = sfg::Window::Create(dialog_style);
		window->SetTitle("New Level");
		std::weak_ptr<sfg::Window> weak_window = window;
		window->GetSignal(sfg::Window::OnCloseButton).Connect([weak_window, this] {
			_gui.Remove(weak_window.lock());
		});

		//box for all the window elements to go in
		auto window_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		//main box is for the top half of the dialog
		auto main_box = sfg::Box::Create();
		//these two box's go in the main box and store the two labels and entries
		auto label_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		auto entry_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		//selectors for terrainset and type
		auto terrain_label_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		auto terrain_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		auto preview_box = sfg::Box::Create();
		//bottom box for holding the button to activate the action
		auto bottom_box = sfg::Box::Create();

		auto width_label = sfg::Label::Create("Width:");
		auto height_label = sfg::Label::Create("Height:");
		label_box->PackEnd(width_label);
		label_box->PackEnd(height_label);

		auto width_entry = sfg::Entry::Create(hades::to_string(MapSize.x));
		width_entry->SetRequisition({ 40.f, 0.f });
		auto height_entry = sfg::Entry::Create(hades::to_string(MapSize.y));
		height_entry->SetRequisition({ 40.f, 0.f });

		entry_box->PackEnd(width_entry);
		entry_box->PackEnd(height_entry);

		auto terrainset_label = sfg::Label::Create("Terrain Set:");
		auto terrain_label = sfg::Label::Create("Terrain:");
		terrain_label_box->PackEnd(terrainset_label);
		terrain_label_box->PackEnd(terrain_label);

		//add terrain selector
		auto terrainset_selector = sfg::ComboBox::Create();
		std::weak_ptr<sfg::ComboBox> terrainset_selector_weak = terrainset_selector;
		for (const auto &tset : resources::TerrainSets)
		{
			const auto name = hades::data::GetAsString(tset);
			terrainset_selector->AppendItem(name);
		}

		auto terrain_selector = sfg::ComboBox::Create();
		std::weak_ptr<sfg::ComboBox> t_selector_weak = terrain_selector;
		std::weak_ptr<sfg::Box> preview_weak = preview_box;

		auto on_terrain_select = [preview_weak, t_selector_weak, terrainset_selector_weak] {
			auto preview_box = preview_weak.lock();
			auto terrain_sel = t_selector_weak.lock();
			auto terrainset_sel = terrainset_selector_weak.lock();
			const auto terrainset_selected = terrainset_sel->GetSelectedItem();

			assert(terrainset_selected < static_cast<int>(resources::TerrainSets.size()));
			const auto terrain_id = resources::TerrainSets[terrainset_selected];
			const auto terrainset = hades::data::Get<resources::terrainset>(terrain_id);

			const auto selected = terrain_sel->GetSelectedItem();

			assert(selected < static_cast<int>(terrainset->terrains.size()));

			const auto terrain = terrainset->terrains[selected];
			assert(!terrain->full.empty());

			const auto t = terrain->full[0];
			const auto tile_settings = tiles::GetTileSettings();
			//create an animation to pass to 'CreateButton'
			hades::resources::animation a;
			a.tex = t.texture;
			a.height = a.width = tile_settings->tile_size;
			hades::resources::animation_frame f{ t.left, t.top, 1.f };
			a.value.push_back(f);

			auto button = objects::editor::CreateButton("preview", nullptr, &a);

			preview_box->RemoveAll();
			preview_box->PackEnd(button);
		};

		terrain_selector->GetSignal(sfg::ComboBox::OnSelect).Connect(on_terrain_select);

		auto on_terrainset_select = [t_selector_weak, terrainset_selector_weak, on_terrain_select]
		{
			auto terrainset_sel = terrainset_selector_weak.lock();
			auto terrain_sel = t_selector_weak.lock();

			const auto selected = terrainset_sel->GetSelectedItem();
			assert(selected < static_cast<int>(resources::TerrainSets.size()));

			while (terrain_sel->GetItemCount())
				terrain_sel->RemoveItem(0);

			const auto terrain_set = hades::data::Get<resources::terrainset>(resources::TerrainSets[selected]);

			for (const auto &t : terrain_set->terrains)
				terrain_sel->AppendItem(hades::data::GetAsString(t->id));

			terrain_sel->SelectItem(0);
			on_terrain_select();
		};

		terrainset_selector->GetSignal(sfg::ComboBox::OnSelect).Connect(on_terrainset_select);

		auto preview_button = objects::editor::CreateButton("preview", nullptr, nullptr);
		preview_box->PackEnd(preview_button);

		terrainset_selector->SelectItem(0);
		on_terrainset_select();

		terrain_box->PackEnd(terrainset_selector);
		terrain_box->PackEnd(terrain_selector);

		main_box->PackEnd(label_box);
		main_box->PackEnd(entry_box);
		main_box->PackEnd(terrain_label_box);
		main_box->PackEnd(terrain_box);
		main_box->PackEnd(preview_box);
		auto button = sfg::Button::Create("Create");
		std::weak_ptr<sfg::Entry> weak_width = width_entry, weak_height = height_entry;
		button->GetSignal(sfg::Button::OnLeftClick).Connect([weak_width, weak_height, weak_window, this] {
			auto width = weak_width.lock();
			auto height = weak_height.lock();
			assert(width && height);

			try
			{
				auto x = std::stoi(width->GetText().toAnsiString());
				auto y = std::stoi(height->GetText().toAnsiString());

				if (x < 1
					|| y < 1)
					return MakeErrorDialog("Map width or height too small");

				MapSize = { x, y };

				auto window = weak_window.lock();
				_gui.Remove(window);

				NewLevel();
				reinit();
			}
			catch (std::invalid_argument&)
			{
				//could not convert width/height to integer
				MakeErrorDialog("Unable to convert width or height to a number.");
			}
			catch (std::out_of_range&)
			{
				//couldn't fit calculated number within an int.
				MakeErrorDialog("Width or height too large. Max: " + hades::to_string(std::numeric_limits<int>::max()));
			}
		});

		bottom_box->PackEnd(button);

		window_box->PackEnd(main_box);
		window_box->PackEnd(bottom_box);
		window->Add(window_box);
		objects::PlaceWidgetInCentre(*window);
		_gui.Add(window);
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