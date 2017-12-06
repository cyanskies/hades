#ifndef TILES_EDITOR_HPP
#define TILES_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Tiles/resources.hpp"
#include "Tiles/tiles.hpp"

//a simple tile editor
//this allows the creation of tile levels with objects placed on them

namespace tiles
{
	namespace editor
	{
		const hades::types::string tile_editor_layout = "editor-layout",
			tile_selector_panel = "tile-selector";

		//screen view height
		// set editor_height to override this
		const hades::types::int32 view_height = 240;

		enum EditMode {
			NONE, //no editing
			TILE, //draw a specific tile, no cleanup
			TILE_EDIT_END
		};
	}

	class tile_editor_base : public hades::State
	{
	public:
		virtual void init() override; 

		void generate();
		
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
		void FillTileSelector();

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
	private:
		void _new(const hades::types::string&, const hades::types::string&, tile_count_t width, tile_count_t height);
		void _load(const hades::types::string&, const hades::types::string&);
		void _save() const;

		//preview drawing variables
		sf::Vector2u _tilePosition;
		MutableTileMap _tilePreview;

		//tile placement mode variables
		tile _tileInfo;
		hades::types::uint8 _tile_draw_size = 1;

		//core map drawing variables
		sf::View _gameView;
	};

	template<class MapClass = tiles::MutableTileMap>
	class tile_editor : public tile_editor_base
	{
	public:
		virtual void load_map(const MapData&) override;
		virtual MapData save_map() const override;

		virtual void draw(sf::RenderTarget &target, sf::Time deltaTime) override;
	protected:
		virtual void GenerateDrawPreview(const sf::RenderTarget&, const hades::InputSystem::action_set&) override;
		virtual void TryDraw(const hades::InputSystem::action_set&) override;

		//core map variables
		MapClass Map;
	};
}

#include "Tiles/editor.inl"

#endif // !TILES_EDITOR_HPP
