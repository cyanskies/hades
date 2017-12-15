#ifndef OBJECT_EDITOR_HPP
#define OBJECT_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Objects/resources.hpp"

//this allows the creation of levels with objects placed on them

namespace objects
{
	namespace editor
	{
		const hades::types::string object_editor_layout = "editor-layout",
			object_selector_panel = "object-selector";

		//screen view height
		// set editor_height to override this
		const hades::types::int32 view_height = 240;

		enum EditMode {
			OBJECT,
			OBJECT_MODE_END
		};

		enum ObjectMode {
			SELECT,
			DRAG,
			PLACE
		};
	}

	class object_editor : public hades::State
	{
	public:
		void init() override; 

		virtual void generate();
		
		virtual void load_map(const MapData&) = 0;
		virtual MapData save_map() const = 0;
		
		virtual bool handleEvent(const hades::Event &windowEvent) override; 
		virtual void update(sf::Time deltaTime, const sf::RenderTarget&, hades::InputSystem::action_set) override;
		virtual void draw(sf::RenderTarget &target, sf::Time deltaTime) override = 0;

		virtual void cleanup() override;
						
		virtual void reinit() override;
		virtual void pause() override;
		virtual	void resume() override;

	protected:
		//==initialisation functions==
		//loads the gui from the editor-layout resource
		//then sets up all the generic UI elements
		void CreateGui();
		//fills the 'tiles' gui containers 
		//with selectable tiles.
		virtual void FillTileSelector();

		virtual sf::FloatRect GetMapBounds() const = 0;
		//==map editing functions==
		//generates the current tile that will be pasted
		virtual void GenerateDrawPreview(const sf::RenderTarget&, const hades::InputSystem::action_set&) = 0;
		//check and draw the new tiles over the current map
		virtual void TryDraw(const hades::InputSystem::action_set&) = 0;
		
		//map file info
		hades::types::string Mod, Filename;

		//editing variables
		using EditMode_t = hades::types::uint8;
		EditMode_t EditMode;
		const resources::tile_settings *TileSettings;

		//tile placement mode variables
		hades::types::uint8 Tile_draw_size = 1;
		tile TileInfo;

		//core map drawing variables
		sf::View GameView;
	private:
		void _new(const hades::types::string&, const hades::types::string&, tile_count_t width, tile_count_t height);
		void _load(const hades::types::string&, const hades::types::string&);
		void _save() const;
	};
}

#endif // !OBJECT_EDITOR_HPP
