#include "Objects/editor.hpp"

#include "TGUI/Animation.hpp"
#include "TGUI/Widgets/ChildWindow.hpp"
#include "TGUI/Widgets/ComboBox.hpp"
#include "TGUI/Widgets/MenuBar.hpp"
#include "TGUI/Widgets/Button.hpp"
#include "TGUI/Widgets/EditBox.hpp"
#include "TGUI/Widgets/HorizontalWrap.hpp"
#include "TGUI/Widgets/Label.hpp"
#include "TGUI/Widgets/Panel.hpp"
#include "TGUI/Widgets/Picture.hpp"

#include "Hades/Animation.hpp"
#include "Hades/common-input.hpp"
#include "Hades/Data.hpp"
#include "Hades/Logging.hpp"
#include "Hades/Properties.hpp"

#include "Objects/resources.hpp"

//map size in tiles
const hades::types::uint32 map_height = 100, map_width = 100;

//scrolling values
const hades::types::int32 scroll_margin = 20,
					scroll_rate = 4;

namespace objects
{
	namespace editor
	{

		//TODO: the only way to accept function as reference without blocking 
		//lambdas is to move this to the header and accept the function by template
		tgui::ClickableWidget::Ptr MakeObjectButton(hades::types::string name, OnClickFunc func, const hades::resources::animation *icon)
		{
			tgui::ClickableWidget::Ptr p;
			//TODO: editor-button-size console vars
			auto button_size = 40;
			if (icon)
			{
				auto [tex, x ,y , w, h] = hades::animation::GetFrame(icon, sf::Time::Zero);
				auto texture = tgui::Texture(tex->value, { x, y, w, h });
				p = tgui::Picture::create(texture);
				//TODO: create button pressed effect for icon buttons
			}
			else
				p = tgui::Button::create(name);

			p->setSize({ button_size, button_size });
			
			//if we have a name set it as a tooltip
			if (!name.empty())
			{
				auto panel = tgui::Panel::create();
				auto lbl = tgui::Label::create(name);
				panel->add(lbl);
				panel->setSize({tgui::bindWidth(lbl), tgui::bindHeight(lbl)});
				p->setToolTip(panel);
			}

			if(func)
				p->onClick.connect(func);

			return p;
		}
		
		tgui::ClickableWidget::Ptr MakeObjectButton(hades::types::string name, const hades::resources::animation *icon)
		{
			return MakeObjectButton(name, OnClickFunc(), icon);
		}
	}

	void object_editor::init()
	{
		_object_snap = hades::console::GetInt(editor_snaptogrid, editor_snap_default);
		_gridEnabled = hades::console::GetBool(editor_grid_enabled, true);
		_gridMinSize = hades::console::GetInt(editor_grid_size, editor_grid_default);
		_gridMaxSize = hades::console::GetInt(editor_grid_max, 1);
		_gridCurrentScale = hades::console::GetInt(editor_grid_size_multiple, 1);

		_gridCurrentSize = std::min(_gridCurrentScale->load(), _gridMaxSize->load()) * *_gridMinSize;

		_grid.setCellSize(_gridCurrentSize);

		reinit();
	}

	void object_editor::loadLevel(const hades::types::string &mod, const hades::types::string &filename)
	{
	}

	bool object_editor::handleEvent(const hades::Event &windowEvent)
	{ 
		return false; 
	}

	void object_editor::update(sf::Time deltaTime, const sf::RenderTarget& window, hades::InputSystem::action_set input) 
	{
		static auto window_width = hades::console::GetInt("vid_width", 640),
			window_height = hades::console::GetInt("vid_height", 480);

		auto mousePos = input.find(hades::input::PointerPosition);
		assert(mousePos != std::end(input));
		if (mousePos->active)
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

			//clamp the gameview after moving it
			viewPosition.x = std::clamp(viewPosition.x, 0.f, static_cast<decltype(viewPosition.x)>(MapSize.x));
			viewPosition.y = std::clamp(viewPosition.y, 0.f, static_cast<decltype(viewPosition.y)>(MapSize.y));

			GameView.setCenter(viewPosition);
			_grid.set2dDrawArea({ viewPosition - GameView.getSize() / 2.f, GameView.getSize() });

			GenerateDrawPreview(window, { mousePos->x_axis, mousePos->y_axis });
		}

		auto mouseLeft = input.find(hades::input::PointerLeft);
		assert(mouseLeft != std::end(input));
		if (mouseLeft->active)
		{
			if (!_pointerLeft)
			{
				OnClick({ mouseLeft->x_axis, mouseLeft->y_axis });
				_pointerLeft = true;
			}
			else if (_pointerLeft)
			{
				OnDragStart({ mouseLeft->x_axis, mouseLeft->y_axis });
				_pointerDrag = true;
			}
			else if (_pointerDrag)
			{
				OnDrag({ mouseLeft->x_axis, mouseLeft->y_axis });
			}
		}
		else if (_pointerLeft && _pointerDrag)
		{
			OnDragEnd({ mouseLeft->x_axis, mouseLeft->y_axis });
			_pointerLeft = _pointerDrag = false;
		}
	}

	void object_editor::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		DrawBackground(target);
		DrawObjects(target);
		DrawGrid(target);
		DrawPreview(target);
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
		sf::Vector2f size = { *cheight * screenRatio, static_cast<float>(*cheight) };
		GameView.setSize(size);
		_backgroundView.reset({ { 0.f, 0.f }, size });
		GameView.setCenter({ 100.f,100.f });

		//tell the grid to draw the viewable area
		_grid.set2dDrawArea({ GameView.getCenter() - GameView.getSize() / 2.f, GameView.getSize() });

		_editorBackground.setSize({ static_cast<float>(*wwidth), static_cast<float>(*wheight) });
		_editorBackground.setFillColor(sf::Color(127, 127, 127, 255));

		_mapBackground.setSize(static_cast<sf::Vector2f>(MapSize));
		_mapBackground.setFillColor(sf::Color::Black);

		//TODO: set grid color from editor settings
		_grid.setSize(static_cast<sf::Vector2f>(MapSize));

		_createGui();
	}

	void object_editor::pause() {}
	void object_editor::resume() {}

	const hades::types::string object_button_container = "object-button-container";

	void object_editor::FillGui()
	{
		auto editor_settings = hades::data::Get<resources::editor>(hades::data::GetUid("editor"));

		//===================
		// Add ToolBar Icons
		//===================
		//get the toolbar container
		auto toolbar_panel = _gui.get<tgui::Container>(editor::toolbar_panel);

		//empty mouse button
		auto empty_button = editor::MakeObjectButton("empty", [this]() {
			EditMode = editor::EditMode::OBJECT;
			_objectMode = editor::ObjectMode::NONE_SELECTED;
		}, editor_settings->selection_mode_icon);

		toolbar_panel->add(empty_button);

		//=========================
		// Toolbar Grid Settings
		//=========================
		if (editor_settings->show_grid_settings)
		{
			//show grid toggle
			auto grid_toggle = editor::MakeObjectButton("grid toggle", [this]() {
				hades::console::SetProperty(editor_grid_enabled, !*_gridEnabled);
			}, editor_settings->grid_show_icon);

			toolbar_panel->add(grid_toggle);

			//if the max size is the same as the smallest size 
			//then dont show the size related buttons
			if (*_gridMaxSize > 1)
			{
				//shrink grid size
				auto grid_shrink = editor::MakeObjectButton("grid shrink", [this]() {
					*_gridCurrentScale = std::max(--*_gridCurrentScale, 1);
					_gridCurrentSize = *_gridCurrentScale * *_gridMinSize;
					_grid.setCellSize(_gridCurrentSize);
				}, editor_settings->grid_shrink_icon);

				toolbar_panel->add(grid_shrink);

				//expand grid size
				auto grid_grow = editor::MakeObjectButton("grid grow", [this]() {
					*_gridCurrentScale = std::min(++*_gridCurrentScale, _gridMaxSize->load());
					_gridCurrentSize = *_gridCurrentScale * *_gridMinSize;
					_grid.setCellSize(_gridCurrentSize);
				}, editor_settings->grid_shrink_icon);

				toolbar_panel->add(grid_grow);
			}

			//if object snap isn't being force enabled/disabled,
			//then place a button to toggle it
			if (*_object_snap == SnapToGrid::GRIDSNAP_DISABLED
				|| *_object_snap == SnapToGrid::GRIDSNAP_ENABLED)
			{
				//snap to grid button
				auto grid_snap_button = editor::MakeObjectButton("grid snap", [this]() {
					*_object_snap = !*_object_snap;
				}, editor_settings->grid_snap_icon);

				toolbar_panel->add(grid_snap_button);
			}
		}

		//==============
		// Add Objects
		//==============
		auto object_sel_panel = _gui.get<tgui::Container>(editor::object_selector_panel);
		
		//add the object group combobox
		auto object_combox = tgui::ComboBox::create();
		auto groups = resources::ObjectGroups;
		
		object_combox->setSize({ "&.w", 20 });

		const auto all_str = "all";

		object_combox->addItem(all_str);

		//add all the group names
		for (auto g : groups)
			object_combox->addItem(g.name);

		//rig up the function for filling the object list
		object_combox->onItemSelect.connect([this, all_str](sf::String str) { 
			auto groups = resources::ObjectGroups;
			auto g = std::find_if(std::begin(groups), std::end(groups),
				[str](resources::object_group group) {return group.name == str; });

			if (g != std::end(groups))
				_addObjects(g->obj_list);
			else if (str == all_str)
				_addObjects(resources::Objects);
		});

		object_sel_panel->add(object_combox);

		auto object_container = tgui::HorizontalWrap::create();
		object_container->setSize({ "&.w", tgui::bindHeight(object_sel_panel) - tgui::bindHeight(object_combox) });
		object_sel_panel->add(object_container, object_button_container);

		object_combox->setSelectedItem(all_str);
	}

	void object_editor::GenerateDrawPreview(const sf::RenderTarget &target, MousePos m_pos)
	{
		if (EditMode != editor::EditMode::OBJECT)
			return;
	
		// if an object is held for placement
		// then we draw the preview in the nearest valid position
		// rounded to the nearest pixel pos if editor-snap = false
		// otherwise rounded to &game_grid_size
		if (_objectMode == editor::ObjectMode::PLACE
			|| _objectMode == editor::ObjectMode::DRAG)
		{
			sf::Transformable *object = nullptr;
			static auto size_id = hades::data::GetUid("size");
			auto[size_c, size_v] = GetCurve(_heldObject, size_id);

			hades::resources::curve_types::vector_int size;

			if (size_c && size_v.set)
			{
				size = std::get<hades::resources::curve_types::vector_int>(size_v.value);
				assert(size.size() == 2);
			}
			else //TODO: default object size in editor settings
				size = { 8, 8 };

			//if we have one or more idle animations then place the dummy represented by them
			if (auto anims = GetEditorAnimations(_heldObject); !anims.empty())
			{
				sf::Sprite s;
				
				auto index = hades::random(0u, anims.size() - 1);
				hades::animation::Apply(anims[index], 0.f, s);
				_objectPreview = s;
				object = &std::get<sf::Sprite>(_objectPreview);
			}
			// otherwise we place an appropriatly sized rect
			else
			{
				sf::RectangleShape r;
				r.setSize({ static_cast<float>(size[0]), static_cast<float>(size[1]) });
				r.setFillColor(sf::Color::Cyan);
				r.setOutlineColor(sf::Color::Blue);
				_objectPreview = r;
				object = &std::get<sf::RectangleShape>(_objectPreview);
			}

			if (*_object_snap <= SnapToGrid::GRIDSNAP_DISABLED)
			{
				//snap to pixel
				auto world_pos = hades::pointer::ConvertToWorldCoords(target, 
					{ std::get<0>(m_pos), std::get<1>(m_pos) }, GameView);

				auto max_x = MapSize.x - size[0],
					max_y = MapSize.y - size[1];

				auto pos_x = std::clamp(world_pos.x, 0.f, static_cast<float>(max_x));
				auto pos_y = std::clamp(world_pos.y, 0.f, static_cast<float>(max_y));

				object->setPosition({ pos_x, pos_y });
			}
			else
			{
				//snap to grid enabled
				//places objects in the top left of the nearest grid cell
				auto world_coord = hades::pointer::ConvertToWorldCoords(target, {std::get<0>(m_pos), std::get<1>(m_pos)}, GameView);
				auto snapped_coords = hades::pointer::SnapCoordsToGrid(static_cast<sf::Vector2i>(world_coord), _gridCurrentSize);
		
				//keep the object within the map bounds
				snapped_coords.x = std::clamp(snapped_coords.x, 0, MapSize.x - size[0]);
				snapped_coords.y = std::clamp(snapped_coords.y, 0, MapSize.y - size[1]);
				
				object->setPosition(static_cast<sf::Vector2f>(snapped_coords));
			}
		}
	}

	void object_editor::OnClick(object_editor::MousePos pos)
	{}

	void object_editor::OnDragStart(MousePos pos)
	{}

	void object_editor::OnDrag(MousePos pos)
	{}

	void object_editor::OnDragEnd(MousePos pos)
	{}

	namespace menu_names
	{
		const auto menu_bar = "MenuBar";

		const auto file_menu = "File";

		const auto new_menu = "New...";
		const auto load_menu = "Load...";
		const auto save_menu = "Save";
		const auto save_as_menu = "Save...";

		const auto view_menu = "View";

		const auto reset_gui = "Reset Gui";
	}

	namespace dialog_names
	{
		const auto new_dialog = "new_dialog"; // <-- this should move to the header too
		const auto load = "load_dialog";
		const auto save = "save_dialog";
	}

	//TODO: move these names to the header, so they can be used by others
	namespace new_dialog
	{
		const auto button = "new_button";
		const auto mod = "new_mod";
		const auto filename = "new_filename";
		const auto size_x = "new_sizex";
		const auto size_y = "new_sizey";
	}

	void object_editor::OnMenuClick(sf::String menu)
	{
		if (menu == menu_names::new_menu)
		{
			auto new_dialog = _gui.get<tgui::Container>(dialog_names::new_dialog);
			new_dialog->show();
		}
		else if (menu == menu_names::load_menu)
		{
			auto load_dialog = _gui.get<tgui::Container>(dialog_names::load);
			load_dialog->show();
		}
		else if (menu == menu_names::save_menu)
		{
			SaveLevel();
		}
		else if (menu == menu_names::save_as_menu)
		{
			auto save_dialog = _gui.get<tgui::Container>(dialog_names::save);
			save_dialog->show();
		}
		else if (menu == menu_names::reset_gui)
			reinit();
	}

	bool object_editor::ObjectValidLocation() const
	{
		return false;
	}

	void object_editor::NewLevel()
	{}

	void object_editor::SaveLevel() const
	{}

	void object_editor::DrawBackground(sf::RenderTarget &target) const
	{
		target.setView(_backgroundView);
		target.draw(_editorBackground);
		target.setView(GameView);
		target.draw(_mapBackground);
	}

	void object_editor::DrawObjects(sf::RenderTarget &target) const 
	{
		//TODO:
	}

	void object_editor::DrawGrid(sf::RenderTarget &target) const 
	{
		target.draw(_grid);
		//TODO draw highlighted grid square
	}

	void object_editor::DrawPreview(sf::RenderTarget &target) const
	{
		//if we're in the drag or place mode
		//then draw the current preview
		if (EditMode == editor::EditMode::OBJECT
			&& (_objectMode == editor::ObjectMode::DRAG
				|| _objectMode == editor::ObjectMode::PLACE))
		{
			std::visit([&target](auto &&drawable){
				target.draw(drawable);
			}, _objectPreview);
		}
	}

	void object_editor::_createGui()
	{
		//====================
		//set up the object_editor UI
		//====================
		
		// remove any gui that might currently be loaded
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
		auto menu_bar = _gui.get<tgui::MenuBar>(menu_names::menu_bar);

		menu_bar->moveToFront();
	
		menu_bar->onMenuItemClick.connect([this](const sf::String s) { OnMenuClick(s); });

		//TODO: everything below here:
		//		check the result of the _gui.get functions(return nullptr on failure)
		//		handle nullptr properly as users could provide malformed
		//		editor layouts to trigger null pointer exceptions and access violations

		//===============
		//FILE LOADING UI
		//===============
		auto load_dialog_container = _gui.get<tgui::ChildWindow>(dialog_names::load);
		//hide the container
		load_dialog_container->hide();
		load_dialog_container->onMinimize.connect([](std::shared_ptr<tgui::ChildWindow> w) {
			w->hide();
		});

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
		load_dialog_button->onPress.connect([this]() {
			auto filename = _gui.get<tgui::EditBox>("load_filename");
			auto modname = _gui.get<tgui::EditBox>("load_mod");

			hades::types::string mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			hades::types::string file = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();
			loadLevel(mod, file);

			auto load_container = _gui.get<tgui::Container>(dialog_names::load);
			load_container->hide();
		});

		//===============
		//SAVE UI
		//===============
		auto save_dialog_container = _gui.get<tgui::ChildWindow>(dialog_names::save);
		//hide the container
		save_dialog_container->hide();
		save_dialog_container->onMinimize.connect([](std::shared_ptr<tgui::ChildWindow> w) {
			w->hide();
		});

		//set the default name to be the currently saved file
		auto save_dialog_filename = save_dialog_container->get<tgui::EditBox>("save_filename");
		if (!Filename.empty())
			save_dialog_filename->setText(Filename);
		else
			Filename = save_dialog_filename->getDefaultText();

		auto save_dialog_modname = save_dialog_container->get<tgui::EditBox>("save_mod");
		if (!Mod.empty())
			save_dialog_modname->setText(Mod);
		else
			Mod = save_dialog_modname->getDefaultText();

		//rig up the save button
		auto save_dialog_button = save_dialog_container->get<tgui::Button>("save_button");
		save_dialog_button->onPress.connect([this]() {
			auto filename = _gui.get<tgui::EditBox>("save_filename");
			auto modname = _gui.get<tgui::EditBox>("save_mod");

			Mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			Filename = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();
			SaveLevel();

			auto save_container = _gui.get<tgui::Container>(dialog_names::save);
			save_container->hide();
		});

		//==========
		//New map UI
		//==========
		auto new_dialog_container = _gui.get<tgui::ChildWindow>(dialog_names::new_dialog);
		//hide the container
		new_dialog_container->hide();
		new_dialog_container->onMinimize.connect([](std::shared_ptr<tgui::ChildWindow> w) {
			w->hide();
		});

		//rig up the save button
		auto new_dialog_button = new_dialog_container->get<tgui::Button>(new_dialog::button);
		new_dialog_button->onPress.connect([this]() {
			auto filename = _gui.get<tgui::EditBox>(new_dialog::filename);
			auto modname = _gui.get<tgui::EditBox>(new_dialog::mod);

			auto mod = modname->getText().isEmpty() ? modname->getDefaultText() : modname->getText();
			auto file = filename->getText().isEmpty() ? filename->getDefaultText() : filename->getText();

			auto size_x = _gui.get<tgui::EditBox>(new_dialog::size_x);
			auto size_y = _gui.get<tgui::EditBox>(new_dialog::size_y);

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

			auto new_container = _gui.get<tgui::Container>(dialog_names::new_dialog);
			new_container->hide();
		});
	}

	void object_editor::_addObjects(std::vector<const resources::object*> objects)
	{
		auto container = _gui.get<tgui::Container>(object_button_container);

		container->removeAllWidgets();

		for (auto o : objects)
		{
			auto b = editor::MakeObjectButton(hades::data::GetAsString(o->id), [this, o]() {
				_setHeldObject(o);
			}, o->editor_icon);

			container->add(b);
		}
	}

	void object_editor::_setHeldObject(const resources::object *o)
	{
		EditMode = editor::EditMode::OBJECT;
		_objectMode = editor::ObjectMode::PLACE;
		_heldObject = o;
	}
}