#ifndef OBJECT_EDITOR_HPP
#define OBJECT_EDITOR_HPP

#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Sprite.hpp"

#include "TGUI/Widgets/ClickableWidget.hpp"

#include "Hades/GridArea.hpp"
#include "Hades/Properties.hpp"
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
			DRAG, // dragging is taking place
			PLACE // an object has been chosen from the picker and is being held by the mouse
		};

		using OnClickFunc = std::function<void(void)>;

		tgui::ClickableWidget::Ptr MakeObjectButton(hades::types::string name, const hades::resources::animation *icon = nullptr, OnClickFunc = OnClickFunc());
	}

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
		//responds to user input(place object, place terrain tile, etc)
		virtual void OnClick(MousePos);
		//respond to mouse drag
		virtual void OnDragStart(MousePos);
		virtual void OnDrag(MousePos);
		virtual void OnDragEnd(MousePos);
		//override to check if a location on the map is valid for the provided object
		//ie. depending on terrain, or other objects
		virtual bool ObjectValidLocation() const;
		//function for creating a new level
		virtual void NewLevel();
		//function for saving the level, call provided Save<NAME> functions
		virtual void SaveLevel() const;
		//these also save and load the map size parameters
		void SaveObjects(level &l) const;
		void LoadObjects(const level &l);

		void DrawBackground(sf::RenderTarget &target) const;
		void DrawObjects(sf::RenderTarget &target) const;
		void DrawGrid(sf::RenderTarget &target) const;

		//map file info
		hades::types::string Mod = "./", Filename = "new.lvl";
		sf::Vector2i MapSize = { 200, 200 };

		//editing variables
		using EditMode_t = hades::types::uint8;
		EditMode_t EditMode = editor::OBJECT;
		
		//core map drawing variables
		sf::View GameView;
	private:
		//loads the gui from the editor-layout resource
		//then sets up all the generic UI elements
		void _createGui();
		void _newMap();
		void _addObjects(std::vector<const resources::object*> objects);
		//Sets the object that will be placed on left click
		void _setHeldObject(const resources::object*);
		//when the mouse is released this object will be placed in the pointers position
		void _setDragObject(const resources::object* o);
		editor::ObjectMode _objectMode = editor::ObjectMode::NONE_SELECTED;

		//object placement and drawing
		const resources::object *_heldObject = nullptr;
		std::variant<sf::RectangleShape,
			sf::Sprite> _objectPreview;
		hades::console::property_bool _object_snap;

		//the limits of the pointer scroll
		hades::types::int32 _pointer_min_x;
		hades::types::int32 _pointer_min_y;
		//true if mouse was down in the previous frame
		bool _pointerLeft = false, _pointerDrag = false;
		//drawing the out of bounds background
		//and grid
		sf::View _backgroundView;
		sf::RectangleShape _editorBackground, _mapBackground;
		hades::console::property_int _gridMinSize;
		hades::types::int32 _gridCurrentSize;
		hades::GridArea _grid;
	};
}

#endif // !OBJECT_EDITOR_HPP
