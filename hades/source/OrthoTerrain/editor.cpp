#include "OrthoTerrain/editor.hpp"

#include <cmath> //std::round
#include <fstream>
#include <thread> //see: FillTileList()
				  // async building of tile selector ui

#include "TGUI/Animation.hpp"
#include "TGUI/HorizontalWrap.hpp"
#include "TGUI/Widgets/ChildWindow.hpp"
#include "TGUI/Widgets/ComboBox.hpp"
#include "TGUI/Widgets/EditBox.hpp"
#include "TGUI/Widgets/Picture.hpp"
#include "TGUI/Widgets/MenuBar.hpp"

#include "yaml-cpp/emitter.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"

#include "Hades/common-input.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/files.hpp"
#include "Hades/Properties.hpp"
#include "Hades/StandardPaths.hpp"

#include "OrthoTerrain/generator.hpp"
#include "OrthoTerrain/resources.hpp"
#include "OrthoTerrain/serialise.hpp"

using namespace hades;

const auto terrain_editor_window = "editor-window";
const auto terrain_editor_tabs = "editor-tabs";
const auto terrain_editor_layout = "editor-layout";

//map size in tiles
const hades::types::uint32 map_height = 100, map_width = 100;

//scrolling values
const hades::types::int32 scroll_margin = 20,
scroll_rate = 4;

namespace ortho_terrain
{
	void terrain_editor::init()
	{
		auto settings_id = hades::data_manager->getUid(ortho_terrain::resources::terrain_settings_name);

		if (!data_manager->exists(settings_id))
			LOGERROR("Missing important settings for orthographic terrain");

		try
		{
			_tile_settings = hades::data_manager->get<ortho_terrain::resources::terrain_settings>(settings_id);
		}
		catch (data::resource_null &e)
		{
			LOGERROR("Tile Settings hasn't been defined: " + std::string(e.what()));
			kill();
			return;
		}

		//_filename is empty if the editor is created without a target file to open
		if (Filename.empty())
			generate();
		else
			_load(Mod, Filename);

		reinit();
	}

	void terrain_editor::generate()
	{
		//TODO: handle lack of terrains
		auto terrain = hades::data_manager->get<resources::terrain>(resources::Terrains[1]);
		const auto map = generator::Blank({ map_height, map_width }, terrain);
		_terrain.create(map);
	}

	void terrain_editor::load(const OrthoSave& data)
	{
		std::vector<hades::data::UniqueId> tilesets;
		tilesets.reserve(data.tilesets.size());

		for (const auto &tileset : data.tilesets)
			tilesets.push_back(hades::data_manager->getUid(std::get<types::string>(tileset)));

		auto map_data = CreateMapData(data);
		_terrain.create(map_data);
		_loaded = true;
	}

	OrthoSave terrain_editor::save_terrain() const
	{
		return CreateOrthoSave(_terrain.getMap());
	}

	bool terrain_editor::handleEvent(const hades::Event &windowEvent)
	{ 
		if (std::get<sf::Event>(windowEvent).type == sf::Event::EventType::Resized)
		{
			//call reinit in resize in order to keep the view accurate
			reinit();
			return true;
		}
		return false; 
	}

	void terrain_editor::update(sf::Time deltaTime, const sf::RenderTarget& window, hades::InputSystem::action_set input) 
	{
		static auto window_width = console::GetInt("vid_width", 640),
			window_height = console::GetInt("vid_height", 480);

		auto mousePos = input.find(hades::input::PointerPosition);
		if (mousePos != input.end() && mousePos->active)
		{
			//move the view
			if (mousePos->x_axis < scroll_margin)
				_gameView.move({ -static_cast<float>(scroll_rate), 0.f });
			else if (mousePos->x_axis > *window_width - scroll_margin)
				_gameView.move({ static_cast<float>(scroll_rate), 0.f });

			if (mousePos->y_axis < scroll_margin)
				_gameView.move({ 0.f, -static_cast<float>(scroll_rate) });
			else if (mousePos->y_axis > *window_height - scroll_margin)
				_gameView.move({ 0.f, static_cast<float>(scroll_rate) });

			auto viewPosition = _gameView.getCenter();
			auto terrainSize = _terrain.getLocalBounds();
			//clamp the gameview after moving it
			if (viewPosition.x < terrainSize.left)
				viewPosition.x = terrainSize.left;
			else if (viewPosition.x > terrainSize.left + terrainSize.width)
				viewPosition.x = terrainSize.left + terrainSize.width;

			if (viewPosition.y < terrainSize.top)
				viewPosition.y = terrainSize.top;
			else if (viewPosition.y > terrainSize.top + terrainSize.height)
				viewPosition.y = terrainSize.top + terrainSize.height;		

			_gameView.setCenter(viewPosition);

			if (_editMode != editor::EditMode::NONE)
				GenerateTerrainPreview(window, input);
		}

		if (_editMode != editor::EditMode::NONE)
			PasteTerrain(input);
	}
	
	void terrain_editor::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		target.setView(_gameView);
		target.draw(_terrain);

		if (_editMode != editor::EditMode::NONE)
			target.draw(_terrainPlacement);
	}

	void terrain_editor::cleanup(){}

	void terrain_editor::reinit() 
	{
		//=========================
		//set up the editor rendering
		//=========================
		//generate the view
		auto cheight = hades::console::GetInt("editor_height", editor::view_height);

		//current window size
		auto wwidth = console::GetInt("vid_width", 640),
			wheight = console::GetInt("vid_height", 480);

		float screenRatio = *wwidth / static_cast<float>(*wheight);

		//set the view
		_gameView.setSize(*cheight * screenRatio, static_cast<float>(*cheight));
		_gameView.setCenter({ 100.f,100.f });

		//====================
		//set up the terrain_editor UI
		//====================

		_gui.removeAllWidgets();

		//==================
		//load the terrain_editor UI
		//==================
		auto layout_id = data_manager->getUid(terrain_editor_layout);
		if (!data_manager->exists(layout_id))
		{
			LOGERROR("No GUI layout exists for " + std::string(terrain_editor_layout));
			kill();
			return;
		}

		auto layout = data_manager->getString(layout_id);
		try
		{
			_gui.loadWidgetsFromStream(std::stringstream(layout->value));
		}
		catch (tgui::Exception &e)
		{
			LOGERROR(e.what());
			kill();
			return;
		}

		//pass all available terrain to the tile picker UI
		FillTileList(resources::Terrains, resources::Tilesets);

		//===========
		//add menubar
		//===========
		//NOTE: cannot be specified in data files. :(
		// maybe in a later version of TGUI

		auto menu_bar = tgui::MenuBar::create();

		const auto file_menu = "File";
		const auto new_menu = "New...";
		const auto load_menu = "Load";
		const auto save_menu = "Save";

		//add menu entries;
		menu_bar->addMenu(file_menu);
		menu_bar->addMenuItem(file_menu, new_menu);
		menu_bar->addMenuItem(file_menu, load_menu);
		menu_bar->addMenuItem(file_menu, save_menu);
		_gui.add(menu_bar);

		auto fadeTime = sf::milliseconds(100);

		menu_bar->connect("MenuItemClicked", [this, file_menu, new_menu, load_menu, save_menu, fadeTime](std::vector<sf::String> menu) {
			//check the options available
			//menu[0] == file
			assert(menu.size() == 2);
			if (menu[0] == file_menu)
			{
				if (menu[1] == new_menu)
				{
					auto new_dialog = _gui.get<tgui::Container>("new_dialog", true);
					new_dialog->showWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
				}
				else if (menu[1] == load_menu)
				{
					auto load_dialog = _gui.get<tgui::Container>("load_dialog", true);
					load_dialog->showWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
				}
				else if (menu[1] == save_menu)
					_save();
			}
		});

		//===============
		//FILE LOADING UI
		//===============
		auto load_dialog_container = _gui.get<tgui::Container>("load_dialog", true);
		//hide the container
		load_dialog_container->hide();

		//set the default name to be the currently loaded file
		auto load_dialog_filename = load_dialog_container->get<tgui::EditBox>("load_filename", true);
		if (!Filename.empty())
			load_dialog_filename->setText(Filename);
		else
			Filename = load_dialog_filename->getDefaultText();

		auto load_dialog_modname = load_dialog_container->get<tgui::EditBox>("load_mod", true);
		if (!Mod.empty())
			load_dialog_modname->setText(Mod);
		else
			Mod = load_dialog_modname->getDefaultText();

		//rig up the load button
		auto load_dialog_button = load_dialog_container->get<tgui::Button>("load_button", true);
		load_dialog_button->connect("pressed", [this, fadeTime]() {
			auto filename = _gui.get<tgui::EditBox>("load_filename", true);
			auto modname = _gui.get<tgui::EditBox>("load_mod", true);

			types::string mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			types::string file = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();
			_load(mod, file);

			auto load_container = _gui.get<tgui::Container>("load_dialog", true);
			load_container->hideWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
		});

		//==========
		//New map UI
		//==========
		auto new_dialog_container = _gui.get<tgui::Container>("new_dialog", true);
		//hide the container
		new_dialog_container->hide();

		//add generators
		auto new_generator = new_dialog_container->get<tgui::ComboBox>("new_gen", true);
		new_generator->addItem("generator_blank", "Blank");

		//rig up the load button
		auto new_dialog_button = new_dialog_container->get<tgui::Button>("new_button", true);
		new_dialog_button->connect("pressed", [this, fadeTime]() {
			auto filename = _gui.get<tgui::EditBox>("new_filename", true);
			auto modname = _gui.get<tgui::EditBox>("new_mod", true);

			auto mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			auto file = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();

			auto size_x = _gui.get<tgui::EditBox>("new_sizex", true);
			auto size_y = _gui.get<tgui::EditBox>("new_sizey", true);

			auto size_x_str = size_x->getText().isEmpty() ? size_x->getDefaultText() : size_x->getText();
			auto size_y_str = size_y->getText().isEmpty() ? size_y->getDefaultText() : size_y->getText();

			auto value_x = std::stoi(size_x_str.toAnsiString());
			auto value_y = std::stoi(size_y_str.toAnsiString());
			
			if (value_x < 1)
				value_x = 1;

			if (value_y < 1)
				value_y = 1;

			//only one generator is available, so we don't really need to check it.
			_new(mod, file, value_x, value_y);

			auto new_container = _gui.get<tgui::Container>("new_dialog", true);
			new_container->hideWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
		});
	}

	void terrain_editor::pause() {}
	void terrain_editor::resume() {}

	void terrain_editor::GenerateTerrainPreview(const sf::RenderTarget& window, const hades::InputSystem::action_set &input)
	{
		auto mousePos = input.find(hades::input::PointerPosition);
		if (mousePos != input.end() && mousePos->active)
		{
			const auto tile_size = _tile_settings->tile_size;

			auto truePos = window.mapPixelToCoords({ mousePos->x_axis, mousePos->y_axis }, _gameView);
			truePos += {static_cast<float>(tile_size), static_cast<float>(tile_size)};

			if (_editMode == editor::EditMode::TILE)
			{
				auto snapPos = truePos - sf::Vector2f(static_cast<float>(std::abs(std::fmod(truePos.x, tile_size))),
					static_cast<float>(std::abs((std::fmod(truePos.y, tile_size)))));
				auto position = sf::Vector2u(snapPos) / tile_size;
				if (_terrainPosition != position)
				{
					_terrainPosition = position;
					_terrainPlacement = _terrain;
					_terrainPlacement.replace(_tileInfo, _terrainPosition, _tile_draw_size, true);
				}
			}
			else if (_editMode == editor::EditMode::TERRAIN)
			{
				auto vertPos = sf::Vector2f{ std::round(truePos.x / tile_size), std::round(truePos.y / tile_size) };
				_generatePreview(_terrainInfo, static_cast<sf::Vector2u>(vertPos),
					_terrain_draw_size);
			}
		}
	}

	//pastes the currently selected tile or terrain onto the map
	void terrain_editor::PasteTerrain(const hades::InputSystem::action_set &input)
	{
		auto mouseLeft = input.find(input::PointerLeft);
		if (mouseLeft != input.end() && mouseLeft->active)
		{
			if (_editMode == editor::EditMode::TILE)
			{
				//place the tile in the tile map
				_terrain.replace(_tileInfo, _terrainPosition, _tile_draw_size, true);
			}
			else if (_editMode == editor::EditMode::TERRAIN)
			{
				_terrain.replace(*_terrainInfo, _terrainPosition, _terrain_draw_size);
			}
		}
	}

	void terrain_editor::FillTileList(const std::vector<hades::data::UniqueId> &terrains, const std::vector<hades::data::UniqueId> &tilesets)
	{
		auto settings_id = hades::data_manager->getUid(ortho_terrain::resources::terrain_settings_name);

		if (!data_manager->exists(settings_id))
			LOGERROR("Missing important settings for orthographic terrain");

		const auto settings = hades::data_manager->get<ortho_terrain::resources::terrain_settings>(settings_id);

		auto tile_size = settings->tile_size;
		auto tile_button_size = tile_size * 3;

		//=============================
		//fill the tile selector window
		//=============================
		static const auto tilesize_label = "tile-size";
		auto tileSize = _gui.get<tgui::EditBox>(tilesize_label, true);
		if (tileSize)
		{
			tileSize->connect("TextChanged", [this, tileSize]() {
				try
				{
					auto value = std::stoi(tileSize->getText().toAnsiString());
					if (value >= 0)
						_tile_draw_size = value;
				}
				//throws invalid_argument and out_of_range, 
				//we can't do anything about either of these
				//leave the tile edit size the same
				catch (...)
				{
					return;
				}
			});
		}

		static const auto tile_container = "tiles";
		auto tileContainer = _gui.get<tgui::Container>(tile_container, true);

		if (!tileContainer)
		{
			LOGERROR("Colony terrain_editor Gui failed, missing container for 'tiles'");
			kill();
			return;
		}

		auto tileLayout = tgui::HorizontalWrap::create();
		tileLayout->setSize("&.w", "&.h");

		std::vector<tile> used_tiles;

		auto make_tiles = [this, &used_tiles, tileLayout, tile_size, tile_button_size](std::vector<ortho_terrain::tile> tiles) {
			for (auto &t : tiles)
			{
				//skip if needed data is missing.
				if (!data_manager->exists(t.texture))
				{
					continue;
					LOGERROR("terrain_editor UI skipping tile because texture is missing: " + data_manager->as_string(t.texture));
				}

				auto texture = data_manager->getTexture(t.texture);
				auto tex = tgui::Texture(texture->value, { static_cast<int>(t.left), static_cast<int>(t.top),
					static_cast<int>(tile_size), static_cast<int>(tile_size) });

				auto tileButton = tgui::Picture::create();
				tileButton->setTexture(tex);
				tileButton->setSize(tile_button_size, tile_button_size);

				tileButton->connect("clicked", [this, t, tile_size]() {
					_editMode = editor::EditMode::TILE;
					_tileInfo = t;
				});

				tileLayout->add(tileButton);
			}
		};

		auto data_mutex = std::make_shared<std::mutex>();
		std::thread tile_work([tileLayout, tileContainer, tilesets, settings, make_tiles, data_mutex]() {
			//place available tiles in the "tiles" container
			for (auto t : tilesets)
			{
				//TODO: make sure error tile isn't included
				std::lock_guard<std::mutex> lock(*data_mutex);
				if (data_manager->exists(t) && t != settings->error_tile)
				{
					auto tileset = data_manager->get<ortho_terrain::resources::tileset>(t);
					assert(tileset);

					//for each tile in the terrain, add it to the tile picker
					make_tiles(tileset->tiles);
				}
			}// !for tilesets

			tileContainer->add(tileLayout);
		});

		 //================================
		 //fill the terrain selector window
		 //================================

		static const auto terrain_size = "terrain-size";
		auto terrainSize = _gui.get<tgui::EditBox>(terrain_size, true);
		if (terrainSize)
		{
			terrainSize->connect("TextChanged", [this, terrainSize]() {
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

		static const auto terrain_container = "terrain";
		auto terrainContainer = _gui.get<tgui::Container>(terrain_container, true);

		if (!terrainContainer)
		{
			LOGERROR("Colony terrain_editor Gui failed, missing container for 'terrain'");
			kill();
			return;
		}

		auto terrainLayout = tgui::HorizontalWrap::create();
		terrainLayout->setSize("&.w", "&.h");
		
		std::thread terrain_work([this, terrainLayout, terrainContainer, terrains, settings, tile_button_size, data_mutex]() {
			//place available terrain in the "terrain" container
			for (auto t : terrains)
			{
				std::lock_guard<std::mutex> lock(*data_mutex);
				if (data_manager->exists(t) && t != settings->error_tile)
				{
					auto terrain = data_manager->get<ortho_terrain::resources::terrain>(t);
					assert(terrain);

					auto tile = terrain->tiles[0];

					//skip if needed data is missing.
					if (!data_manager->exists(tile.texture))
					{
						continue;
						LOGERROR("terrain_editor UI skipping tile because texture is missing: " + data_manager->as_string(tile.texture));
					}

					auto texture = data_manager->getTexture(terrain->tiles[0].texture);
					auto tex = tgui::Texture(texture->value, { static_cast<int>(tile.left), static_cast<int>(tile.top),
						static_cast<int>(settings->tile_size), static_cast<int>(settings->tile_size) });

					auto tileButton = tgui::Picture::create();
					tileButton->setTexture(tex);
					tileButton->setSize(tile_button_size, tile_button_size);

					tileButton->connect("clicked", [this, terrain]() {
						_editMode = editor::EditMode::TERRAIN;
						_terrainInfo = terrain;
					});

					terrainLayout->add(tileButton);
				}//if exists

			}//for Terrains
			terrainContainer->add(terrainLayout);
		});

		tile_work.detach();
		terrain_work.detach();
	}

	void terrain_editor::_generatePreview(ortho_terrain::resources::terrain *terrain, sf::Vector2u vert_position,
		hades::types::uint8 size)
	{
		if (vert_position == _terrainPosition)
			return;

		_terrainPosition = vert_position;

		_terrainPlacement = _terrain;
		_terrainPlacement.replace(*terrain, vert_position, size);
	}

	void terrain_editor::_new(const hades::types::string& mod, const hades::types::string& filename, tile_count_t width, tile_count_t height)
	{
		//TODO: handle case of missing terrain
		auto terrain = hades::data_manager->get<resources::terrain>(resources::Terrains[1]);
		const auto map = generator::Blank({ width, height }, terrain);
		_terrain.create(map);

		Mod = mod;
		Filename = filename;
	}

	void terrain_editor::_load(const hades::types::string& mod, const hades::types::string& filename)
	{
		auto file = hades::files::as_string(mod, filename);
		auto yamlRoot = YAML::Load(file);

		auto saveData = Load(yamlRoot);
		load(saveData);

		Mod = mod;
		Filename = filename;
	}

	void terrain_editor::_save() const
	{
		//generate the output string
		const auto saveData = save_terrain();

		YAML::Emitter output;
		output << YAML::BeginMap;
		output << saveData;
		output << YAML::EndMap;

		//write over the target
		const auto &target = hades::GetUserCustomFileDirectory() + Mod + Filename;

		std::ofstream file(target, std::ios_base::out);

		file << output.c_str();

		LOG("Wrote map to file: " + target);
	}
}