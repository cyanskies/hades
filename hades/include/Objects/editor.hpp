#ifndef OBJECT_EDITOR_HPP
#define OBJECT_EDITOR_HPP

#include "TGUI/Widgets/ClickableWidget.hpp"

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
			object_selector_panel = "object-selector";

		//screen view height
		// set editor_height to override this
		const hades::types::int32 view_height = 240;

		enum EditMode {
			OBJECT,
			OBJECT_MODE_END
		};

		enum ObjectMode {
			NONE, // nothing selected
			SELECT, // something selected
			DRAG, // dragging is taking place
			PLACE // an object has been chosen from the picker and is being held by the mouse
		};

		tgui::ClickableWidget::Ptr MakeObjectButton(hades::types::string name, const hades::resources::animation *icon = nullptr);
	}

	class object_editor : public hades::State
	{
	public:
		virtual void init() override; 

		// called when the load command is issued
		// overide to parse extra elements from the level
		// then call the LoadX functions to parse all the subsequent parts
		virtual void loadLevel();

		virtual bool handleEvent(const hades::Event &windowEvent) override; 
		virtual void update(sf::Time deltaTime, const sf::RenderTarget&, hades::InputSystem::action_set) override;
		virtual void draw(sf::RenderTarget &target, sf::Time deltaTime) override;

		virtual void cleanup() override;
						
		virtual void reinit() override;
		virtual void pause() override;
		virtual	void resume() override;

	protected:
		//==initialisation functions==
		//fills the 'tiles' gui containers 
		//with selectable tiles.
		virtual void FillGui();

		sf::Vector2i GetMapBounds() const;
		//==map editing functions==
		//generates the preview to be drawn over the map
		virtual void GenerateDrawPreview(const sf::RenderTarget&, const hades::InputSystem::action_set&);
		virtual void OnClick();
		virtual void SaveLevel() const;
		//these also save and load the map size parameters
		void SaveObjects(level &l) const;
		void LoadObjects(const level &l);

		void DrawBackground(sf::RenderTarget &target) const;
		void DrawObjects(sf::RenderTarget &target) const;

		//map file info
		hades::types::string Mod, Filename;

		//editing variables
		using EditMode_t = hades::types::uint8;
		EditMode_t EditMode = editor::OBJECT;
		
		//core map drawing variables
		sf::View GameView;
	private:
		//loads the gui from the editor-layout resource
		//then sets up all the generic UI elements
		void _createGui();
		void _addObjects(std::vector<const resources::object*> objects);
		void _setHeldObject(const resources::object*);
		EditMode_t _objectMode = editor::NONE;

		sf::Vector2i _mapSize;
	};
}

#endif // !OBJECT_EDITOR_HPP
