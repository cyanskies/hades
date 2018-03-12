#include "Objects/editor.hpp"

#include "SFML/Graphics/RenderTexture.hpp"

#include "SFGUI/Widgets.hpp"

#include "Hades/Animation.hpp"
#include "Hades/common-input.hpp"
#include "Hades/Data.hpp"
#include "Hades/Logging.hpp"
#include "Hades/Properties.hpp"

#include "Objects/resources.hpp"

//scrolling values
const hades::types::int32 scroll_margin = 20,
					scroll_rate = 4;

bool ShowSelector(objects::object_editor::EditMode_t mode, objects::editor::ObjectMode objMode)
{
	using namespace objects::editor;
	return mode == EditMode::OBJECT
		&& (objMode == ObjectMode::SELECT
			|| objMode == ObjectMode::DRAG);
 }

namespace objects
{
	void object_editor::init()
	{
		//access the console properties used by the editor
		//editor settings
		//object settings
		_object_snap = hades::console::GetInt(editor_snaptogrid, editor_snap_default);
		//grid settings
		_gridEnabled = hades::console::GetBool(editor_grid_enabled, true);
		_gridMinSize = hades::console::GetInt(editor_grid_size, editor_grid_default);
		_gridMaxSize = hades::console::GetInt(editor_grid_max, 1);

		_gridCurrentScale = hades::console::GetInt(editor_grid_size_multiple, 1);
		_gridCurrentSize = std::min(_gridCurrentScale->load(), _gridMaxSize->load()) * *_gridMinSize;
		_grid.setCellSize(_gridCurrentSize);

		//set up the selection and collision variables
		_quadtree = QuadTree({ 0, 0, MapSize.x, MapSize.y }, 1);
		_objectSelector.setFillColor(sf::Color::Transparent);
		_objectSelector.setOutlineColor(sf::Color::White);
		_objectSelector.setOutlineThickness(1.f);

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

		const auto mousePos = input.find(hades::input::PointerPosition);
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
			_grid.limitDrawTo({ viewPosition - GameView.getSize() / 2.f, GameView.getSize() });

			//update grid highlight location
			_updateGridHighlight(window, { mousePos->x_axis, mousePos->y_axis });

			//generate placement draw preview
			GenerateDrawPreview(window, { mousePos->x_axis, mousePos->y_axis });
		}

		const auto drag_time = sf::milliseconds(50);
		const auto mouseLeft = input.find(hades::input::PointerLeft);
		assert(mouseLeft != std::end(input));
		if (mouseLeft->active)
		{
			//this is the first frame the mouse has been down for
			if (_mouseLeftDown == MouseState::MOUSE_UP)
			{
				//record where the mouse was when first pressed
				_mouseDownPos = { mouseLeft->x_axis, mouseLeft->y_axis };
				_mouseLeftDown = MouseState::MOUSE_DOWN;
				_mouseDownTime.restart();
			}
			else if(_mouseLeftDown == MouseState::MOUSE_DOWN
				&& _mouseDownTime.getElapsedTime() > drag_time)
			{
				//no draggable object under curser
				if (ValidTargetForDrag(_mouseDownPos))
				{
					_mouseLeftDown = MouseState::DRAG_MOVE;
					OnDragStart(_mouseDownPos);
				}
				else // no object to select for drag
					_mouseLeftDown = MouseState::DRAG_DRAW;
			}
			else if (_mouseLeftDown == MouseState::DRAG_DRAW)
			{
				OnClick(window, { mouseLeft->x_axis, mouseLeft->y_axis });
			}
		}
		else if (!mouseLeft->active)
		{
			//mouse was being held,  but is now released
			//this is a click if the held time was less the drag time
			if (_mouseLeftDown == MouseState::MOUSE_DOWN)
			{
				OnClick(window, _mouseDownPos);
				_mouseLeftDown = MouseState::MOUSE_UP;
			}
		}

		if (EditMode != editor::EditMode::OBJECT
			|| !(_objectMode == editor::ObjectMode::SELECT || _objectMode == editor::ObjectMode::DRAG))
			_clearObjectSelected();
	}

	void object_editor::draw(sf::RenderTarget &target, sf::Time deltaTime)
	{
		DrawBackground(target);
		DrawGrid(target);
		DrawObjects(target);
		DrawPreview(target);
	}

	void object_editor::cleanup(){}

	void object_editor::reinit() 
	{
		//=========================
		//set up the editor rendering
		//=========================
		//generate the view
		const auto cheight = hades::console::GetInt("editor_height", editor::view_height);

		//current window size
		const auto wwidth = hades::console::GetInt("vid_width", 800);
		const auto wheight = hades::console::GetInt("vid_height", 600);

		const float screenRatio = *wwidth / static_cast<float>(*wheight);

		//set the view
		sf::Vector2f size = { *cheight * screenRatio, static_cast<float>(*cheight) };
		GameView.setSize(size);
		_backgroundView.reset({ { 0.f, 0.f }, size });
		GameView.setCenter({ 100.f,100.f });

		//tell the grid to draw the viewable area
		_grid.limitDrawTo({ GameView.getCenter() - GameView.getSize() / 2.f, GameView.getSize() });

		_editorBackground.setSize({ static_cast<float>(*wwidth), static_cast<float>(*wheight) });
		_editorBackground.setFillColor(sf::Color(127, 127, 127, 255));

		_mapBackground.setSize(static_cast<sf::Vector2f>(MapSize));
		_mapBackground.setFillColor(sf::Color::Black);

		//TODO: set grid color from editor settings
		_grid.setSize(static_cast<sf::Vector2f>(MapSize));

		EditMode = editor::EditMode::OBJECT;
		_objectMode = editor::ObjectMode::NONE_SELECTED;

		_createGui();
	}

	void object_editor::pause() {}
	void object_editor::resume() {}

	void object_editor::FillToolBar(AddToggleButtonFunc ToggleButton, AddButtonFunc Button, AddSeperatorFunc Seperator)
	{
		const auto editor_settings = hades::data::Get<resources::editor>(hades::data::GetUid("editor"));
		//common editor items
		Button("new map", [this]() {
			NewLevelDialog();
		}, nullptr);

		Button("save map", [this]() {
			SaveLevelDialog();
		}, nullptr);

		Button("load map", [this]() {
			LoadLevelDialog();
		}, nullptr);

		Button("exit", [this]() {
			//TODO: add warning dialog
			kill();
		}, nullptr);

		//empty mouse button
		Button("selector", [this]() {
			EditMode = editor::EditMode::OBJECT;
			_objectMode = editor::ObjectMode::NONE_SELECTED;
			_clearObjectSelected();
		}, editor_settings->selection_mode_icon);

		//=========================
		// Toolbar Grid Settings
		//=========================

		//add seperator to sperate the grid buttons from the generic editor buttons
		Seperator();

		if (editor_settings->show_grid_settings)
		{
			//show grid toggle
			Button("grid toggle", [this]() {
				hades::console::SetProperty(editor_grid_enabled, !*_gridEnabled);
			}, editor_settings->grid_show_icon);

			//if the max size is the same as the smallest size 
			//then dont show the size related buttons
			if (*_gridMaxSize > 1)
			{
				//shrink grid size
				Button("grid shrink", [this]() {
					*_gridCurrentScale = std::max(--*_gridCurrentScale, 1);
					_gridCurrentSize = *_gridCurrentScale * *_gridMinSize;
					_grid.setCellSize(_gridCurrentSize);
				}, editor_settings->grid_shrink_icon);

				//expand grid size
				Button("grid grow", [this]() {
					*_gridCurrentScale = std::min(++*_gridCurrentScale, _gridMaxSize->load());
					_gridCurrentSize = *_gridCurrentScale * *_gridMinSize;
					_grid.setCellSize(_gridCurrentSize);
				}, editor_settings->grid_grow_icon);
			}

			//if object snap isn't being force enabled/disabled,
			//then place a button to toggle it
			if (*_object_snap == SnapToGrid::GRIDSNAP_DISABLED
				|| *_object_snap == SnapToGrid::GRIDSNAP_ENABLED)
			{
				//snap to grid button
				Button("grid snap", [this]() {
					*_object_snap = !*_object_snap;
				}, editor_settings->grid_snap_icon);
			}
		}
	}

	void object_editor::FillGui()
	{
		_onEnterObjectMode();
	}

	sfg::Box::Ptr object_editor::GetPalatteContainer()
	{
		return _paletteWindow;
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
			static const auto size_id = hades::data::GetUid("size");
			static const auto size_curve = hades::data::Get<hades::resources::curve>(size_id);
			const auto size_value = ValidVectorCurve(GetCurve(_heldObject, size_curve));

			using size_type = hades::resources::curve_types::vector_int;
			const auto size = std::get<size_type>(size_value.value);

			//if we have one or more idle animations then place the dummy represented by them
			if (const auto anims = GetEditorAnimations(_heldObject.obj_type); !anims.empty())
			{
				sf::Sprite s;

				const auto index = hades::random(0u, anims.size() - 1);
				const auto anim = anims[index];
				hades::animation::Apply(anim, 0.f, s);
				//calculate scale for x and y axis?
				const auto width_scale = static_cast<float>(size[0]) / static_cast<float>(anim->width);
				const auto height_scale = static_cast<float>(size[1]) / static_cast<float>(anim->height);

				//scale the sprite to match object size
				s.setScale(width_scale, height_scale);
				auto colour = s.getColor();
				colour.a = 255 / 2;
				s.setColor(colour);
				_objectPreview = s;
				object = &std::get<sf::Sprite>(_objectPreview);
			}
			else // otherwise we place an appropriatly sized rect
			{
				sf::RectangleShape r;
				r.setSize({ static_cast<float>(size[0]), static_cast<float>(size[1]) });
				auto colour = sf::Color::Cyan;
				colour.a = 255 / 2;
				r.setFillColor(colour);
				r.setOutlineColor(sf::Color::Blue);
				_objectPreview = r;
				object = &std::get<sf::RectangleShape>(_objectPreview);
			}

			if (*_object_snap <= SnapToGrid::GRIDSNAP_DISABLED)
			{
				//snap to pixel
				const auto world_pos = hades::pointer::ConvertToWorldCoords(target,
					{ std::get<0>(m_pos), std::get<1>(m_pos) }, GameView);

				const auto max_x = MapSize.x - size[0],
					max_y = MapSize.y - size[1];

				const auto pos_x = std::clamp(world_pos.x, 0.f, static_cast<float>(max_x));
				const auto pos_y = std::clamp(world_pos.y, 0.f, static_cast<float>(max_y));

				object->setPosition({ pos_x, pos_y });
			}
			else
			{
				//snap to grid enabled
				//places objects in the top left of the nearest grid cell
				const auto world_coord = hades::pointer::ConvertToWorldCoords(target, { std::get<0>(m_pos), std::get<1>(m_pos) }, GameView);
				auto snapped_coords = hades::pointer::SnapCoordsToGrid(static_cast<sf::Vector2i>(world_coord), _gridCurrentSize);

				//keep the object within the map bounds
				snapped_coords.x = std::clamp(snapped_coords.x, 0, MapSize.x - size[0]);
				snapped_coords.y = std::clamp(snapped_coords.y, 0, MapSize.y - size[1]);

				object->setPosition(static_cast<sf::Vector2f>(snapped_coords));
			}

			if(!ObjectValidLocation(static_cast<sf::Vector2i>(object->getPosition()), _heldObject))
			{
				std::visit([](auto &&obj) {
					using T = std::decay_t < decltype(obj) > ;
					auto col = sf::Color::Red;
					col.a = 255 / 2;
					if constexpr(std::is_same_v<T, sf::Sprite>)
						obj.setColor(col);
					else if constexpr(std::is_same_v<T, sf::RectangleShape>)
						obj.setFillColor(col);
					else
						static_assert(hades::types::always_false<T>::value, "Visitor doesn't handle all cases");
				}, _objectPreview);
			}
		}
	}

	bool object_editor::ValidTargetForDrag(object_editor::MousePos pos) const
	{
		return false;
	}

	void object_editor::OnClick(const sf::RenderTarget &target, object_editor::MousePos pos)
	{
		if (EditMode == editor::EditMode::OBJECT)
		{
			const auto world_pos = hades::pointer::ConvertToWorldCoords(target, { std::get<0>(pos), std::get<1>(pos) }, GameView);
			using int32 = hades::types::int32;
			if (_objectMode == editor::ObjectMode::NONE_SELECTED
				|| _objectMode == editor::ObjectMode::SELECT)
				_trySelectAt({ static_cast<int32>(world_pos.x), static_cast<int32>(world_pos.y) });
			else if(_objectMode == editor::ObjectMode::PLACE)
				_placeHeldObject();
		}
	}

	void object_editor::OnDragStart(MousePos pos)
	{}

	void object_editor::OnDrag(MousePos pos)
	{}

	void object_editor::OnDragEnd(MousePos pos)
	{}

	bool object_editor::ObjectValidLocation(sf::Vector2i position, const object_info &object) const
	{
		//check for collision with another object
		static const auto size_id = hades::data::GetUid("size");
		static const auto size_c = hades::data::Get<hades::resources::curve>(size_id);
		const auto size_v = ValidVectorCurve(GetCurve(object, size_c));
		using size_type = std::vector<hades::resources::curve_types::int_t>;
		const auto obj_size = std::get<size_type>(size_v.value);
		
		const auto bounds = sf::IntRect{ position, { obj_size[0], obj_size[1] } };
		const auto collisions = _quadtree.find_collisions(bounds);

		return std::none_of(std::begin(collisions), std::end(collisions),
			[bounds](auto &&other) { return bounds.intersects(other.rect); });
	}

	void object_editor::NewLevelDialog()
	{}
	void object_editor::SaveLevelDialog()
	{}
	void object_editor::LoadLevelDialog()
	{}

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

	void object_editor::DrawObjects(sf::RenderTarget &target) 
	{
		_objectSprites.prepare();
		target.draw(_objectSprites);

		if (ShowSelector(EditMode, _objectMode))
			target.draw(_objectSelector);
	}

	void object_editor::DrawGrid(sf::RenderTarget &target) const 
	{
		if (_gridEnabled->load())
		{
			target.draw(_grid);
			target.draw(_gridHighlight);
		}
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

	void object_editor::_addToToolBar(sfg::Widget::Ptr w)
	{
		static const auto window_width = hades::console::GetInt("vid_width", 800);
		const auto toolbar_req = _toolBarIconBox->GetAllocation();
		const auto req = w->GetAllocation();
		//if the current line of icons is full
		//add a new line
		if (req.width + toolbar_req.width > *window_width)
		{
			_toolBarIconBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
			_toolBar->PackEnd(_toolBarIconBox);
		}

		_toolBarIconBox->PackEnd(w, false);
	}

	sf::Image GetSubImage(const sf::Texture &i, hades::types::int32 x, hades::types::int32 y,
		hades::types::int32 width, hades::types::int32 height,
		hades::types::int32 target_width, hades::types::int32 target_height)
	{
		sf::RenderTexture rt;
		if (target_width == 0
			|| target_height == 0)
		{
			rt.create(width, height);
		}
		else
		{
			rt.create(target_width, target_height);
			rt.setView(sf::View{ { 0.f, 0.f, static_cast<float>(width), static_cast<float>(height) } });
		}
		
		const sf::Sprite s{ i, {x, y, width, height} };

		rt.clear(sf::Color::Transparent);
		rt.draw(s);
		rt.display();

		const auto &tex = rt.getTexture();
		return tex.copyToImage();
	}

	template <typename Button>
	auto CreateButton(hades::types::string name, object_editor::OnClickFunc func, const hades::resources::animation *icon)
	{
		auto button = Button::Create();

		constexpr auto button_size = 20;

		if (icon)
		{
			using int_t = hades::types::int32;
			const auto[x, y] = hades::animation::GetFrame(icon, sf::Time::Zero);
			const auto frame = GetSubImage(icon->tex->value, static_cast<int_t>(x), static_cast<int_t>(y),
				icon->width, icon->height, button_size, button_size);
			button->SetImage(sfg::Image::Create(frame));
		}
		else
			button->SetLabel(name);

		if (func)
			button->GetSignal(sfg::Widget::OnLeftClick).Connect(func);

		return button;
	}

	void object_editor::_addButtonToToolBar(hades::types::string name, OnClickFunc func, const hades::resources::animation *icon)
	{
		_addToToolBar(CreateButton<sfg::Button>(name, func, icon));
	}

	void object_editor::_addToggleButtonToToolBar(hades::types::string name, OnClickFunc func, const hades::resources::animation *icon)
	{
		_addToToolBar(CreateButton<sfg::ToggleButton>(name, func, icon));
	}

	void object_editor::_addSeparatorToToolBar()
	{
		_addToToolBar(sfg::Separator::Create(sfg::Separator::Orientation::VERTICAL));
	}

	void object_editor::_createGui()
	{
		//====================
		//set up the object_editor UI
		//====================
		
		// remove any gui that might currently be loaded
		_gui.RemoveAll();

		static const auto window_width = hades::console::GetInt("vid_width", 800);

		//====================
		// ToolBar
		//====================
		using Style = sfg::Window::Style;
		const auto toolbar_style = Style::BACKGROUND;
		auto toolbar_window = sfg::Window::Create(toolbar_style);
		const auto toolbar_height = 20.f;
		toolbar_window->SetRequisition({ static_cast<float>(*window_width), toolbar_height });
		_toolBar = sfg::Box::Create();
		_toolBarIconBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
		_toolBar->PackEnd(_toolBarIconBox, false);
		toolbar_window->Add(_toolBar);
		_gui.Add(toolbar_window);

		//give decendants a chance to add to the toolbar
		FillToolBar([this](hades::types::string s, OnClickFunc f, const hades::resources::animation* a) {
			_addToggleButtonToToolBar(s, f, a);
		}, [this](hades::types::string s, OnClickFunc f, const hades::resources::animation* a) {
			_addButtonToToolBar(s, f, a);
		}, [this] {
			_addSeparatorToToolBar();
		});

		//=============
		// Left Panel
		//=============
		auto window_height = hades::console::GetInt("vid_height", 600);

		auto left_panel = sfg::Window::Create(toolbar_style);
		const auto panel_top = toolbar_window->GetAllocation().height;
		const auto panel_width = 150.f;
		left_panel->SetRequisition({ panel_width, static_cast<float>(*window_height) - panel_top });
		left_panel->SetPosition({ 0.f, panel_top });

		//create the layout for the left panel
		auto left_panel_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		left_panel->Add(left_panel_box);

		_gui.Add(left_panel);

		//Palatte window
		_paletteWindow = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		left_panel_box->PackEnd(_paletteWindow);

		//Property Window
		_propertyWindow = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		left_panel_box->PackEnd(_propertyWindow);

		//let child classes start adding their own elements
		FillGui();
		
		/*
		//===================
		//FILE LOADING DIALOG
		//===================
		//Shown when the Load button is pressed
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
		//SAVE DIALOG
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
		//New map DIALOG
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
		*/
	}


	void object_editor::_onEnterObjectMode()
	{
		//clear the palatte window and build the object palatte gui
		_paletteWindow->RemoveAll();

		auto combobox = sfg::ComboBox::Create();

		//add the selector combobox
		_paletteWindow->PackEnd(combobox, false);

		_objectPalette = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 1.f);

		_paletteWindow->PackEnd(_objectPalette);

		constexpr auto all_str = "all";

		combobox->AppendItem(all_str);

		const auto &groups = objects::resources::ObjectGroups;

		//add all the group names
		for (auto g : groups)
			combobox->AppendItem(g.name);

		std::weak_ptr<sfg::ComboBox> combo_weak{ combobox };
		auto on_select_item = [this, &groups, combo_weak] {
			const auto combo = combo_weak.lock();
			//selected is in the range [0 - groups.size() + 1]
			//[0] is all_string, everything else is groups[selected - 1]
			const auto selected = combo->GetSelectedItem();

			if (selected == 0)
			{
				const auto &g = objects::resources::Objects;
				_addObjects(g);
			}
			else
			{
				assert(static_cast<int>(groups.size()) >= selected);
				const auto &g = groups[selected - 1].obj_list;
				_addObjects(g);
			}
		};

		combobox->GetSignal(sfg::ComboBox::OnSelect).Connect(on_select_item);

		combobox->SelectItem(0);
		on_select_item();

		_clearObjectSelected();
	}

	void object_editor::_addObjects(const std::vector<const resources::object*> &objects)
	{
		//replace the contents of _objectPalette with the objects in objects
		assert(_objectPalette);
		_objectPalette->RemoveAll();

		const auto palette_alloc = _paletteWindow->GetRequisition();
		auto box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
		_objectPalette->PackEnd(box, false, false);

		for(const auto o : objects)
		{
			const auto name = o->editor_icon ? hades::types::string{} : hades::data::GetAsString(o->id);
			auto b = CreateButton<sfg::Button>(name, [this, o]
			{
				_setHeldObject(o);
			}, o->editor_icon);

			const auto alloc = box->GetRequisition();
			const auto button_alloc = b->GetAllocation();

			if (alloc.x + button_alloc.width > palette_alloc.x)
			{
				box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 1.f);
				_objectPalette->PackEnd(box, false);
			}

			box->PackEnd(b, false);
		}
	}

	void object_editor::_updateGridHighlight(const sf::RenderTarget &target, object_editor::MousePos pos)
	{
		if (_gridEnabled->load())
		{
			//snap to grid enabled
			//places objects in the top left of the nearest grid cell
			const auto world_coord = hades::pointer::ConvertToWorldCoords(target, { std::get<0>(pos), std::get<1>(pos) }, GameView);
			auto snapped_coords = hades::pointer::SnapCoordsToGrid(static_cast<sf::Vector2i>(world_coord), _gridCurrentSize);

			const auto size = _gridCurrentSize - 1;

			//keep the object within the map bounds
			snapped_coords.x = std::clamp(snapped_coords.x + 1, 1, MapSize.x - size);
			snapped_coords.y = std::clamp(snapped_coords.y +1, 1, MapSize.y - size);

			const auto fsize = static_cast<float>(size);
			_gridHighlight.setSize({ fsize, fsize });
			_gridHighlight.setPosition(static_cast<sf::Vector2f>(snapped_coords));
			_gridHighlight.setFillColor(sf::Color::Transparent);
			_gridHighlight.setOutlineColor(sf::Color::Green); //TODO: setGridHightlightColour?
			_gridHighlight.setOutlineThickness(1.f);
		}
	}

	void object_editor::_setHeldObject(const resources::object *o)
	{
		EditMode = editor::EditMode::OBJECT;
		_objectMode = editor::ObjectMode::PLACE;
		_heldObject.curves.clear();
		_heldObject.id = hades::NO_ENTITY;
		_heldObject.obj_type = o;
	}

	void object_editor::_placeHeldObject()
	{
		//find object preview position
		auto position = std::visit([](auto &&val) {
			return val.getPosition();
		}, _objectPreview);

		editor_object_info object{ _heldObject };

		if (ObjectValidLocation(static_cast<sf::Vector2i>(position), object))
		{
			//create the object held at the target location
			object.id = ++_next_object_id;

			static const auto size_id = hades::data::GetUid("size");
			static const auto size_c = hades::data::Get<hades::resources::curve>(size_id);
			const auto size_value = ValidVectorCurve(GetCurve(object, size_c));

			const auto size = std::get<hades::resources::curve_types::vector_int>(size_value.value);
			const sf::Vector2i size_vec{ size[0], size[1] };

			_quadtree.insert({ static_cast<sf::Vector2i>(position), size_vec }, object.id);

			//if no animation is set, send nullptr to the SpriteBatch, 
			//it will render an appropriatly sized rect
			const hades::resources::animation *anim = nullptr;
			const auto anims = GetEditorAnimations(_heldObject.obj_type);
			if (!anims.empty())
			{
				auto index = hades::random(0u, anims.size() - 1);
				anim = anims[index];
			}

			object.sprite_id = _objectSprites.createSprite(anim, sf::Time(), 0,
				position, static_cast<sf::Vector2f>(size_vec));

			//record the object position as one of it's curves
			using namespace hades::resources::curve_types;
			static const auto pos_id = hades::data::GetUid("position");
			static const auto pos_c = hades::data::Get<hades::resources::curve>(pos_id);
			auto pos_value = ValidVectorCurve(GetCurve(object, pos_c));
			pos_value.value = vector_int{ static_cast<int_t>(position.x), static_cast<int_t>(position.y) };
			object.curves.push_back({ pos_c, pos_value });

			_objects.push_back(object);
		}
	}

	void object_editor::_trySelectAt(MousePos pos)
	{
		//these should have been checked before calling this func
		assert(EditMode == editor::EditMode::OBJECT);
		assert(_objectMode == editor::ObjectMode::NONE_SELECTED
			|| _objectMode == editor::ObjectMode::SELECT);

		const auto mpos = sf::Vector2i{ std::get<0>(pos), std::get<1>(pos) };
		const auto candidates = _quadtree.find_collisions({ mpos, {1, 1} });

		const auto target = std::find_if(std::begin(candidates), std::end(candidates), [mpos](auto &&r) {
			return r.rect.contains(mpos);
		});

		//nothing was under the cursor
		if (target == std::end(candidates))
			return;

		const auto obj = std::find_if(std::begin(_objects), std::end(_objects), [id = target->key](auto &&o) {
			return o.id == id;
		});

		//this should be 'impossible'
		assert(obj != std::end(_objects));

		_onObjectSelected(*obj);
	}

	void object_editor::_onObjectSelected(editor_object_info &info)
	{
		assert(EditMode == editor::EditMode::OBJECT);
		assert(_objectMode == editor::ObjectMode::NONE_SELECTED 
			|| _objectMode == editor::ObjectMode::SELECT);
		_objectMode = editor::ObjectMode::SELECT;

		_updateInfoBox(info);
		_updateSelector(info);
	}

	constexpr auto property_entry_width = 70.f;

	template<typename OnEnter>
	sfg::Box::Ptr MakeEditRow(std::string_view text, std::string_view value, OnEnter func)
	{
		auto box = sfg::Box::Create();

		using namespace hades;
		auto property_name = sfg::Entry::Create(to_string(text));
		property_name->SetRequisition({ property_entry_width, 0.f });
		property_name->SetState(sfg::Widget::State::INSENSITIVE);
		box->PackEnd(property_name, false);

		auto value_edit = sfg::Entry::Create(to_string(value));
		std::weak_ptr<sfg::Entry> weak_value = value_edit;
		value_edit->GetSignal(sfg::Entry::OnTextChanged).Connect([func, weak_value] {
			auto value = weak_value.lock();
			if (!value)
				return;

			func(value->GetText().toAnsiString());
		});

		value_edit->SetRequisition({ property_entry_width, 0.f });

		box->PackEnd(value_edit, false);

		return box;
	}

	using namespace hades::resources;
	using CurveEntry = std::tuple<const curve*, curve_default_value>;
	sfg::Box::Ptr MakePropertyEditRow(std::string_view text, CurveEntry c, object_info &obj)
	{
		using namespace hades;
		auto [curve, c_value] = c;
		assert(curve);
		const auto value_str = CurveValueToString(c_value);

		auto on_enter = [curve, &obj](auto &&text) {
			const auto value = StringToCurveValue(curve, text);
			auto c = std::find_if(std::begin(obj.curves), std::end(obj.curves), [curve](CurveEntry c) {
				const hades::resources::curve *other_curve = std::get<const hades::resources::curve*>(c);
				return other_curve == curve;
			});

			if (c == std::end(obj.curves))
				obj.curves.emplace_back(std::make_tuple(curve, value));
			else
				std::get<curve_default_value>(*c) = value;
		};

		return MakeEditRow(to_string(text), value_str, on_enter);
	}

	constexpr auto selected = "Selected: ";

	hades::types::string SelectionTitle(std::string_view type_name, std::string_view obj_name)
	{
		hades::types::string message = selected;
		constexpr auto max_name_size = 15;
		if (obj_name.empty())
			message += type_name;
		else
		{
			hades::types::string name{ obj_name };
			if (name.size() > max_name_size)
				name = name.substr(0, max_name_size) + "...";
			message += name + "(" + hades::to_string(type_name) + ")";
		}

		return message;
	}

	void object_editor::_updateInfoBox(object_info &obj)
	{
		const auto obj_type_name = hades::data::GetAsString(obj.obj_type->id);
		
		_propertyWindow->RemoveAll();
		auto label = sfg::Label::Create(SelectionTitle(obj_type_name, obj.name));
		label->SetAlignment({ 0.f, 0.f });
		_propertyWindow->PackEnd(label, false, false);

		//add the immutable entity id
		{
			auto box = sfg::Box::Create();
			auto id_entry = sfg::Entry::Create("Id");
			id_entry->SetState(sfg::Widget::State::INSENSITIVE);
			id_entry->SetRequisition({ property_entry_width, 0.f });
			auto id_value = sfg::Entry::Create(hades::to_string(obj.id));
			id_value->SetState(sfg::Widget::State::INSENSITIVE);
			id_value->SetRequisition({ property_entry_width, 0.f });
			box->PackEnd(id_entry, false);
			box->PackEnd(id_value, false);
			_propertyWindow->PackEnd(box, false, false);
		}

		
		//add object name
		auto &used_names = _usedObjectNames;
		std::weak_ptr<sfg::Label> weak_title = label;
		auto name_change_lamb = [this, &obj, &used_names, weak_title, obj_type_name](const hades::types::string &text) {
			auto &current_value = obj.name;

			if (text == current_value)
				return;

			if (text == "")
			{
				used_names.erase(current_value);
				current_value = text;
				return;
			}
			//if the name has already been used
			if (used_names.find(text) != std::end(used_names))
				return;

			if (!current_value.empty())
				used_names.erase(current_value);

			used_names.insert(text);
			current_value = text;

			auto title = weak_title.lock();
			title->SetText(SelectionTitle(obj_type_name, text));
		};

		auto name_edit = MakeEditRow("Name", obj.name, name_change_lamb);
		_propertyWindow->PackEnd(name_edit, false);
		/*
		//add the special cased position, and size properties
		//add all other properties
		*/
	}

	void object_editor::_updateSelector(const object_info &info)
	{
		using curve = hades::resources::curve;
	
		//set the selector position
		static const auto position_id = hades::data::GetUid("position");
		static const auto position_c = hades::data::Get<curve>(position_id);
		assert(position_c);

		const auto pos_v = ValidVectorCurve(GetCurve(info, position_c));
		using int_vec = hades::resources::curve_types::vector_int;
		const auto &pos = std::get<int_vec>(pos_v.value);

		_objectSelector.setPosition(static_cast<float>(pos[0]), static_cast<float>(pos[1]));

		//set the selectors size
		static const auto size_id = hades::data::GetUid("size");
		static const auto size_c = hades::data::Get<curve>(size_id);
		assert(size_c);

		const auto size_v = ValidVectorCurve(GetCurve(info, size_c));
		const auto &size = std::get<int_vec>(size_v.value);

		_objectSelector.setSize({ static_cast<float>(size[0] + 1), static_cast<float>(size[1] + 1) });
	}

	void object_editor::_clearObjectSelected()
	{
		//_objectMode = editor::ObjectMode::NONE_SELECTED;

		_propertyWindow->RemoveAll();
		static const auto message = "Selected: \"Nothing\"";
		const auto label = sfg::Label::Create(message);
		_propertyWindow->PackEnd(label);
	}
}