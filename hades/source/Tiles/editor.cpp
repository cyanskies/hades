#include "Tiles/editor.hpp"

#include <cmath> //std::round
#include <fstream>
#include <thread> //see: FillTileList()
				  // async building of tile selector ui

#include "TGUI/Animation.hpp"
#include "TGUI/Widgets/HorizontalWrap.hpp"
#include "TGUI/Widgets/ChildWindow.hpp"
#include "TGUI/Widgets/ComboBox.hpp"
#include "TGUI/Widgets/EditBox.hpp"
#include "TGUI/Widgets/Picture.hpp"
#include "TGUI/Widgets/MenuBar.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/common-input.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/files.hpp"
#include "Hades/Properties.hpp"
#include "Hades/StandardPaths.hpp"

#include "Tiles/generator.hpp"
#include "Tiles/resources.hpp"
#include "Tiles/serialise.hpp"
#include "Tiles/tiles.hpp"

using namespace hades;

const auto tile_editor_base_window = "editor-window";
const auto tile_editor_base_tabs = "editor-tabs";

//map size in tiles
const hades::types::uint32 map_height = 100, map_width = 100;

//scrolling values
const hades::types::int32 scroll_margin = 20,
scroll_rate = 4;

namespace tiles
{
	void tile_editor_base::init()
	{
		TileSettings = &GetTileSettings();

		generate();

		reinit();
	}

	void tile_editor_base::generate()
	{
		//TODO: throw if no tiles have been registered
		//TODO: don't use error tiles in this
		auto first_tileset = hades::data_manager->get<resources::tileset>(resources::Tilesets.front());
		const auto map = generator::Blank({ map_height, map_width }, first_tileset->tiles.front());
		load_map(map);
	}

	bool tile_editor_base::handleEvent(const hades::Event &windowEvent)
	{ 
		if (std::get<sf::Event>(windowEvent).type == sf::Event::EventType::Resized)
		{
			//call reinit in resize in order to keep the view accurate
			reinit();
			return true;
		}
		return false; 
	}

	void tile_editor_base::update(sf::Time deltaTime, const sf::RenderTarget& window, hades::InputSystem::action_set input) 
	{
		static auto window_width = console::GetInt("vid_width", 640),
			window_height = console::GetInt("vid_height", 480);

		auto mousePos = input.find(hades::input::PointerPosition);
		if (mousePos != input.end() && mousePos->active)
		{
			//move the view
			if (mousePos->x_axis < scroll_margin)
				GameView.move({ -static_cast<float>(scroll_rate), 0.f });
			else if (mousePos->x_axis > *window_width - scroll_margin)
				GameView.move({ static_cast<float>(scroll_rate), 0.f });

			if (mousePos->y_axis < scroll_margin)
				GameView.move({ 0.f, -static_cast<float>(scroll_rate) });
			else if (mousePos->y_axis > *window_height - scroll_margin)
				GameView.move({ 0.f, static_cast<float>(scroll_rate) });

			auto viewPosition = GameView.getCenter();
			auto terrainSize = GetMapBounds();
			//clamp the gameview after moving it
			if (viewPosition.x < terrainSize.left)
				viewPosition.x = terrainSize.left;
			else if (viewPosition.x > terrainSize.left + terrainSize.width)
				viewPosition.x = terrainSize.left + terrainSize.width;

			if (viewPosition.y < terrainSize.top)
				viewPosition.y = terrainSize.top;
			else if (viewPosition.y > terrainSize.top + terrainSize.height)
				viewPosition.y = terrainSize.top + terrainSize.height;		

			GameView.setCenter(viewPosition);

			if (EditMode != editor::EditMode::NONE)
				GenerateDrawPreview(window, input);
		}

		if (EditMode != editor::EditMode::NONE)
			TryDraw(input);
	}

	void tile_editor_base::cleanup(){}

	void tile_editor_base::reinit() 
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
		GameView.setSize(*cheight * screenRatio, static_cast<float>(*cheight));
		GameView.setCenter({ 100.f,100.f });

		CreateGui();
	}

	void tile_editor_base::pause() {}
	void tile_editor_base::resume() {}

	void tile_editor_base::CreateGui()
	{
		//====================
		//set up the tile_editor_base UI
		//====================

		_gui.removeAllWidgets();

		//==================
		//load the tile_editor_base UI
		//==================
		auto layout_id = data_manager->getUid(editor::tile_editor_layout);
		if (!data_manager->exists(layout_id))
		{
			LOGERROR("No GUI layout exists for " + std::string(editor::tile_editor_layout));
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
		FillTileSelector();

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

		menu_bar->onMenuItemClick.connect([this, file_menu, new_menu, load_menu, save_menu, fadeTime](std::vector<sf::String> menu) {
			//check the options available
			//menu[0] == file
			assert(menu.size() == 2);
			if (menu[0] == file_menu)
			{
				if (menu[1] == new_menu)
				{
					auto new_dialog = _gui.get<tgui::Container>("new_dialog");
					new_dialog->showWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
				}
				else if (menu[1] == load_menu)
				{
					auto load_dialog = _gui.get<tgui::Container>("load_dialog");
					load_dialog->showWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
				}
				else if (menu[1] == save_menu)
					;//_save();
			}
		});

		//===============
		//FILE LOADING UI
		//===============
		auto load_dialog_container = _gui.get<tgui::Container>("load_dialog");
		//hide the container
		load_dialog_container->hide();

		//set the default name to be the currently loaded file
		auto load_dialog_filename = load_dialog_container->get<tgui::EditBox>("load_filename");
		if (!Filename.empty())
			load_dialog_filename->setText(Filename);
		else
			Filename = load_dialog_filename->getDefaultText();

		auto load_dialog_modname = load_dialog_container->get<tgui::EditBox>("load_mod");
		if (!Mod.empty())
			load_dialog_modname->setText(Mod);
		else
			Mod = load_dialog_modname->getDefaultText();

		//rig up the load button
		auto load_dialog_button = load_dialog_container->get<tgui::Button>("load_button");
		load_dialog_button->onPress.connect([this, fadeTime]() {
			auto filename = _gui.get<tgui::EditBox>("load_filename");
			auto modname = _gui.get<tgui::EditBox>("load_mod");

			types::string mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			types::string file = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();
			//_load(mod, file);

			auto load_container = _gui.get<tgui::Container>("load_dialog");
			load_container->hideWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
		});

		//==========
		//New map UI
		//==========
		auto new_dialog_container = _gui.get<tgui::Container>("new_dialog");
		//hide the container
		new_dialog_container->hide();

		//add generators
		auto new_generator = new_dialog_container->get<tgui::ComboBox>("new_gen");
		new_generator->addItem("generator_blank", "Blank");

		//rig up the load button
		auto new_dialog_button = new_dialog_container->get<tgui::Button>("new_button");
		new_dialog_button->onPress.connect([this, fadeTime]() {
			auto filename = _gui.get<tgui::EditBox>("new_filename");
			auto modname = _gui.get<tgui::EditBox>("new_mod");

			auto mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			auto file = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();

			auto size_x = _gui.get<tgui::EditBox>("new_sizex");
			auto size_y = _gui.get<tgui::EditBox>("new_sizey");

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

			auto new_container = _gui.get<tgui::Container>("new_dialog");
			new_container->hideWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
		});
	}

	void tile_editor_base::FillTileSelector()
	{
		const auto &tilesets = resources::Tilesets;

		auto settings_id = hades::data_manager->getUid(resources::tile_settings_name);

		if (!data_manager->exists(settings_id))
			LOGERROR("Missing important settings for orthographic terrain");

		const auto settings = hades::data_manager->get<resources::tile_settings>(settings_id);

		auto tile_size = settings->tile_size;
		auto tile_button_size = tile_size * 3;

		//=============================
		//fill the tile selector window
		//=============================
		static const auto tilesize_label = "draw-size";
		auto tileSize = _gui.get<tgui::EditBox>(tilesize_label);
		if (tileSize)
		{
			tileSize->onTextChange.connect([this, tileSize]() {
				try
				{
					auto value = std::stoi(tileSize->getText().toAnsiString());
					if (value >= 0)
						Tile_draw_size = value;
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

		auto tileContainer = _gui.get<tgui::Container>(editor::tile_selector_panel);

		if (!tileContainer)
		{
			LOGERROR("tile_editor_base Gui failed, missing container for 'tiles'");
			kill();
			return;
		}

		auto tileLayout = tgui::HorizontalWrap::create();

		auto make_tiles = [this, tileLayout, tile_size, tile_button_size](std::vector<tile> tiles) {
			for (auto &t : tiles)
			{
				//skip if needed data is missing.
				if (!data_manager->exists(t.texture))
				{
					continue;
					LOGERROR("tile_editor_base UI skipping tile because texture is missing: " + data_manager->as_string(t.texture));
				}

				auto texture = data_manager->getTexture(t.texture);
				auto tex = tgui::Texture(texture->value, { static_cast<int>(t.left), static_cast<int>(t.top),
					static_cast<int>(tile_size), static_cast<int>(tile_size) });

				auto tileButton = tgui::Picture::create(tex);
				tileButton->setSize(tile_button_size, tile_button_size);

				tileButton->onClick.connect([this, t, tile_size]() {
					EditMode = editor::EditMode::TILE;
					TileInfo = t;
				});

				tileLayout->add(tileButton);
			}
		};

		//TODO: make this a protected variable
		// it isn't being used by the terrain-selector worker
		auto data_mutex = std::make_shared<std::mutex>();
		std::thread tile_work([tileLayout, tileContainer, tilesets, settings, make_tiles, data_mutex]() {
			//place available tiles in the "tiles" container
			for (auto t : tilesets)
			{
				std::lock_guard<std::mutex> lock(*data_mutex);
				if (data_manager->exists(t))
				{
					auto tileset = data_manager->get<resources::tileset>(t);
					assert(tileset);

					//for each tile in the terrain, add it to the tile picker
					make_tiles(tileset->tiles);
				}
			}// !for tilesets

			tileContainer->add(tileLayout);
		});

		tile_work.detach();
	}

	void tile_editor_base::_new(const hades::types::string& mod, const hades::types::string& filename, tile_count_t width, tile_count_t height)
	{
		//TODO: handle case of missing terrain
		auto first_tileset = hades::data_manager->get<resources::tileset>(resources::Tilesets.front());
		const auto map = generator::Blank({ map_height, map_width }, first_tileset->tiles.front());
		load_map(map);
		
		Mod = mod;
		Filename = filename;
	}

	void tile_editor_base::_load(const hades::types::string& mod, const hades::types::string& filename)
	{
		auto file = hades::files::as_string(mod, filename);
		auto yamlRoot = YAML::Load(file);

		auto saveData = Load(yamlRoot);
		auto map_data = CreateMapData(saveData);
		load_map(map_data);

		Mod = mod;
		Filename = filename;
	}

	void tile_editor_base::_save() const
	{
		//generate the output string
		const auto map_data = save_map();
		const auto saveData = CreateMapSaveData(map_data);

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