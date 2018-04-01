#include "Tiles/editor.hpp"

#include "SFGUI/Widgets.hpp"

#include "Hades/common-input.hpp"
#include "Hades/Data.hpp"
#include "Hades/simple_resources.hpp"

namespace tiles
{
	void tile_editor::init()
	{
		const auto tile_setting_id = hades::data::GetUid(resources::tile_settings_name);
		TileSettings = hades::data::Get<resources::tile_settings>(tile_setting_id);	

		auto tileset_iter = resources::Tilesets.begin();
		if (resources::Tilesets.size() > 2)
			++tileset_iter;

		//if the tilesets vector was empty then just leave the default _tileInfo
		if (tileset_iter != std::end(resources::Tilesets))
		{
			const auto tileset = hades::data::Get<resources::tileset>(*tileset_iter);
			if (!tileset->tiles.empty())
				_tileInfo = tileset->tiles[0];
		}

		object_editor::init();
	}

	void tile_editor::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		target.setView(GameView);
		DrawBackground(target);
		target.draw(Map);
		DrawGrid(target);
		DrawObjects(target);

		//DrawPreview(target);
	}

	void tile_editor::FillToolBar(AddToggleButtonFunc toggle, AddButtonFunc button, AddSeperatorFunc seperator)
	{
		object_editor::FillToolBar(toggle, button, seperator);

		seperator();

		button("tiles", [this] {
			EditMode = editor::EditMode::TILE;
			_tileMode = editor::TileEditMode::NONE;
			_addTilesToUi();
		}, nullptr);
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

		if (_tileInfo.texture == hades::UniqueId::Zero)
		{
			//no tile to fill the map with
			_tileInfo = GetErrorTile();
		}

		const tile_count_t width = MapSize.x / TileSettings->tile_size;
		TileArray map{ width * MapSize.y / TileSettings->tile_size, _tileInfo };
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

	void tile_editor::_addTilesToUi()
	{
		assert(EditMode == editor::EditMode::TILE);

		auto palette = GetPaletteContainer();

		palette->RemoveAll();

		auto combobox = sfg::ComboBox::Create();

		//add the selector combobox
		palette->PackEnd(combobox, false);

		_tileWindow = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 1.f);

		palette->PackEnd(_tileWindow);

		constexpr auto all_str = "all";

		combobox->AppendItem(all_str);

		const auto &tilesets = resources::Tilesets;

		//add all the group names
		for (const auto &t : tilesets)
		{
			const auto tset = hades::data::GetAsString(t);
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

				for (const auto &tileset : resources::Tilesets)
				{
					const auto t = hades::data::Get<resources::tileset>(tileset);
					std::copy(std::begin(t->tiles), std::end(t->tiles), std::back_inserter(tiles));
				}

				_addTiles(tiles);
			}
			else
			{
				assert(static_cast<int>(resources::Tilesets.size()) >= selected);
				const auto t = resources::Tilesets[selected - 1];
				const auto tileset = hades::data::Get<resources::tileset>(t);
				_addTiles(tileset->tiles);
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

		const auto palette_alloc = _tileWindow->GetRequisition();
		auto box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
		_tileWindow->PackEnd(box, false, false);

		using objects::editor::CreateButton;

		for (const auto t : tiles)
		{
			//create an animation to pass to 'CreateButton'
			hades::resources::animation a;
			const auto tex = hades::data::Get<hades::resources::texture>(t.texture);
			a.tex = tex;
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
