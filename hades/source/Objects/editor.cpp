#include "Objects/editor.hpp"

#include "TGUI/Animation.hpp"
#include "TGUI/Widgets/MenuBar.hpp"
#include "TGUI/Widgets/Button.hpp"
#include "TGUI/Widgets/EditBox.hpp"

#include "Hades/common-input.hpp"
#include "Hades/Data.hpp"
#include "Hades/Logging.hpp"
#include "Hades/Properties.hpp"

const auto object_editor_window = "editor-window";
const auto object_editor_tabs = "editor-tabs";

//map size in tiles
const hades::types::uint32 map_height = 100, map_width = 100;

//scrolling values
const hades::types::int32 scroll_margin = 20,
					scroll_rate = 4;

namespace objects
{
	void object_editor::init()
	{
		reinit();
	}

	bool object_editor::handleEvent(const hades::Event &windowEvent)
	{ 
		if (std::get<sf::Event>(windowEvent).type == sf::Event::EventType::Resized)
		{
			//call reinit in resize in order to keep the view accurate
			reinit();
			return true;
		}
		return false; 
	}

	void object_editor::update(sf::Time deltaTime, const sf::RenderTarget& window, hades::InputSystem::action_set input) 
	{
		static auto window_width = hades::console::GetInt("vid_width", 640),
			window_height = hades::console::GetInt("vid_height", 480);

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
			viewPosition.x = std::clamp(viewPosition.x, 0.f, static_cast<decltype(viewPosition.x)>(_mapSize.x));
			viewPosition.y = std::clamp(viewPosition.y, 0.f, static_cast<decltype(viewPosition.y)>(_mapSize.y));

			GameView.setCenter(viewPosition);

			GenerateDrawPreview(window, input);
		}

		auto mouseLeft = input.find(hades::input::PointerLeft);
		if (mouseLeft != input.end() && mouseLeft->active)
			OnClick();
	}

	void object_editor::cleanup(){}

	void object_editor::reinit() 
	{
		//=========================
		//set up the editor rendering
		//=========================
		//generate the view
		auto cheight = hades::console::GetInt("editor_height", editor::view_height);

		//current window size
		auto wwidth = hades::console::GetInt("vid_width", 640),
			wheight = hades::console::GetInt("vid_height", 480);

		float screenRatio = *wwidth / static_cast<float>(*wheight);

		//set the view
		GameView.setSize(*cheight * screenRatio, static_cast<float>(*cheight));
		GameView.setCenter({ 100.f,100.f });

		_createGui();
	}

	void object_editor::pause() {}
	void object_editor::resume() {}

	void object_editor::_createGui()
	{
		//====================
		//set up the object_editor UI
		//====================

		_gui.removeAllWidgets();

		//==================
		//load the object_editor UI
		//==================
		auto layout_id = hades::data::GetUid(editor::object_editor_layout);
		if (!hades::data::Exists(layout_id))
		{
			LOGERROR("No GUI layout exists for " + std::string(editor::object_editor_layout));
			kill();
			return;
		}

		auto layout = hades::data::Get<hades::resources::string>(layout_id);
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

		//let child classes start adding their own elements
		FillGui();

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

			//types::string mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			//types::string file = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();
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
		//auto new_generator = new_dialog_container->get<tgui::ComboBox>("new_gen");
		//new_generator->addItem("generator_blank", "Blank");

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
			//_new(mod, file, value_x, value_y);

			auto new_container = _gui.get<tgui::Container>("new_dialog");
			new_container->hideWithEffect(tgui::ShowAnimationType::Fade, fadeTime);
		});
	}
}