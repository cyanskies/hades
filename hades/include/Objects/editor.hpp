#ifndef OBJECT_EDITOR_HPP
#define OBJECT_EDITOR_HPP

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Sprite.hpp"

#include "TGUI/Widgets/ClickableWidget.hpp"

#include "Hades/GridArea.hpp"
#include "Hades/Properties.hpp"
#include "Hades/QuadMap.hpp"
#include "Hades/SpriteBatch.hpp"
#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Objects/Objects.hpp"
#include "Objects/resources.hpp"

//this allows the creation of levels with objects placed on them

namespace objects
{
	namespace editor
	{
		const hades::types::string object_editor_layout = "editor-layout",
			//object selector must be a container, the app will place the object type combobox in it,
			// and objects beneath it.
			object_selector_panel = "object-selector",
			selection_info = "selection-info",
			toolbar_panel = "toolbar-container";

		//screen view height
		// set editor_height to override this
		const hades::types::int32 view_height = 240;

		enum EditMode {
			OBJECT,
			OBJECT_MODE_END
		};

		enum class ObjectMode {
			NONE_SELECTED, // nothing selected
			SELECT, // something selected
			GROUP_SELECT, //
			DRAG, // dragging is taking place
			GROUP_DRAG,
			PLACE // an object has been chosen from the picker and is being held by the mouse
		};

		using OnClickFunc = std::function<void(void)>;

		tgui::ClickableWidget::Ptr MakeObjectButton(hades::types::string name, OnClickFunc, const hades::resources::animation *icon = nullptr);
		tgui::ClickableWidget::Ptr MakeObjectButton(hades::types::string name, const hades::resources::animation *icon = nullptr);
	}

	struct editor_object_info : public object_info
	{
		editor_object_info() = default;
		editor_object_info(const editor_object_info&) = default;
		editor_object_info(editor_object_info&&) = default;
		editor_object_info &operator=(const editor_object_info&) = default;
		editor_object_info &operator=(editor_object_info&&) = default;

		editor_object_info(const object_info& o) : object_info(o)
		{}

		hades::sprite_utility::Sprite::sprite_id sprite_id;
	};

	class object_editor : public hades::State
	{
	public:
		virtual void init() override;

		// called when the load command is issued
		// overide to parse extra elements from the level
		// then call the Protected LoadX functions to parse all the subsequent parts
		virtual void loadLevel(const hades::types::string &mod, const hades::types::string &filename);

		virtual bool handleEvent(const hades::Event &windowEvent) override;
		virtual void update(sf::Time deltaTime, const sf::RenderTarget&, hades::InputSystem::action_set) override final;
		virtual void draw(sf::RenderTarget &target, sf::Time deltaTime) override;

		virtual void cleanup() override;

		virtual void reinit() override;
		virtual void pause() override;
		virtual	void resume() override;

		using EditMode_t = hades::types::uint8;

	protected:
		//==initialisation functions==
		//first call your paren't classes impl of this function
		// then add whatever you want to the gui
		// ToolBar buttons
		// Menu Options
		virtual void FillGui();

		using MousePos = std::tuple<hades::types::int32, hades::types::int32>;
		//==map editing functions==
		//generates the preview to be drawn over the map for the current editing mode
		virtual void GenerateDrawPreview(const sf::RenderTarget&, MousePos);
		//check if the location given contains a valid target for drag moving
		// for this edit mode
		virtual bool ValidTargetForDrag(MousePos) const;
		//responds to user input(place object, place terrain tile, etc)
		virtual void OnClick(MousePos);
		//respond to mouse drag
		virtual void OnDragStart(MousePos);
		virtual void OnDrag(MousePos);
		virtual void OnDragEnd(MousePos);
		virtual void OnMenuClick(sf::String);
		//override to check if a location on the map is valid for the provided object
		//ie. depending on terrain, or other objects
		virtual bool ObjectValidLocation(sf::Vector2i position, const object_info &object) const;
		//function for creating a new level
		virtual void NewLevel();
		//function for saving the level, call provided Save<NAME> functions
		virtual void SaveLevel() const;
		//these also save and load the map size parameters
		void SaveObjects(level &l) const;
		void LoadObjects(const level &l);

		void DrawBackground(sf::RenderTarget &target) const;
		void DrawObjects(sf::RenderTarget &target);
		void DrawGrid(sf::RenderTarget &target) const;
		//draw the preview generated in GenerateDrawPreview
		virtual void DrawPreview(sf::RenderTarget &target) const;

		//map file info
		hades::types::string Mod = "./", Filename = "new.lvl";
		sf::Vector2i MapSize = { 200, 200 };

		//editing variables
		EditMode_t EditMode = editor::OBJECT;

		//core map drawing variables
		sf::View GameView;

	private:
		//loads the gui from the editor-layout resource
		//then sets up all the generic UI elements
		void _createGui();
		void _newMap();
		void _addObjects(std::vector<const resources::object*> objects);
		void _updateGridHighlight(const sf::RenderTarget&, MousePos pos);
		//Sets the object that will be placed on left click
		void _setHeldObject(const resources::object*);
		void _placeHeldObject();
		void _trySelectAt(MousePos pos);
		//sets up the selected object info box and also creates the selection indicator
		//NOTE: selection indicator is controlled by OnDrag when dragging
		void _onObjectSelected(editor_object_info &info);
		//places the selection indicator around the specified object
		void _updateSelector(const object_info &info);
		//clears the selection indicator and selection info box
		//this should be called for any mode other than drag
		void _clearObjectSelected();

		//when the mouse is released this object will be placed in the pointers position
		void _setDragObject(const resources::object* o);
		editor::ObjectMode _objectMode = editor::ObjectMode::NONE_SELECTED;

		//object placement and drawing
		object_info _heldObject;
		std::variant<sf::RectangleShape,
			sf::Sprite> _objectPreview;
		hades::console::property_int _object_snap;
		hades::EntityId _next_object_id = hades::NO_ENTITY;
		//object selection indicator
		sf::RectangleShape _objectSelector;

		//objects in the map
		//id map
		using QuadTree = hades::QuadTree<hades::EntityId>;
		QuadTree _quadtree;
		std::vector<editor_object_info> _objects;

		//sprite batch for objects
		hades::SpriteBatch _objectSprites;

		//the limits of the pointer scroll
		hades::console::property_int _scroll_margin;
		hades::console::property_int _scroll_rate;
		hades::types::int32 _pointer_min_x;
		hades::types::int32 _pointer_min_y;
		//true if mouse was down in the previous frame
		enum class MouseState{MOUSE_UP, MOUSE_DOWN, DRAG_DRAW, DRAG_MOVE};
		MouseState _mouseLeftDown = MouseState::MOUSE_UP;
		MousePos _mouseDownPos;
		sf::Clock _mouseDownTime;
		//drawing the out of bounds background
		//and grid
		sf::View _backgroundView;
		sf::RectangleShape _editorBackground, _mapBackground;
		hades::console::property_int _gridMinSize, _gridCurrentScale, _gridMaxSize;
		hades::console::property_bool _gridEnabled;
		hades::types::int32 _gridCurrentSize;
		sf::RectangleShape _gridHighlight;
		hades::GridArea _grid;
	};
}

#endif // !OBJECT_EDITOR_HPP
