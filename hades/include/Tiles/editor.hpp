#ifndef TILES_EDITOR_HPP
#define TILES_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Tiles/resources.hpp"
#include "Tiles/tiles.hpp"

//a simple tile editor
//this allows the creation of tile levels with objects placed on them
//TODO: object support

namespace tiles
{
	namespace editor
	{
		//screen view height
		// set editor_height to override this
		const hades::types::int32 view_height = 240;

		enum class EditMode {
			NONE, //no editing
			TILE, //draw a specific tile, no cleanup
		};
	}

	class tile_editor : public hades::State
	{
	public:
		tile_editor() = default;
		//generate random map if not already
		void init() override; 

		void generate();
		
		//void load(const OrthoSave&);
		//OrthoSave save_terrain() const;
		
		bool handleEvent(const hades::Event &windowEvent) override; 
		void update(sf::Time deltaTime, const sf::RenderTarget&, hades::InputSystem::action_set) override;
		void draw(sf::RenderTarget &target, sf::Time deltaTime) override;

		void cleanup() override;
						
		void reinit() override;
		void pause() override;
		void resume() override;

	protected:

		//generates the current terrain that will be pasted
		void GenerateTerrainPreview(const sf::RenderTarget&, const hades::InputSystem::action_set&);
		//pastes the currently selected tile or terrain onto the map
		void PasteTerrain(const hades::InputSystem::action_set&);
		//fills the 'terrain' and 'tiles' gui containers 
		//with selectable terrain and tiles.
		//assumes the state _gui object has been already been filled
		void FillTileList(const std::vector<hades::data::UniqueId> &tilesets);

		hades::types::string Mod, Filename;

	private:
		void _new(const hades::types::string&, const hades::types::string&, tile_count_t width, tile_count_t height);
		void _load(const hades::types::string&, const hades::types::string&);
		void _save() const;

		//editor settings
		editor::EditMode _editMode = editor::EditMode::NONE;
		resources::tile_settings *_tile_settings;
		//default save is a file called new.lvl that stored alongside the mod archives
		bool _loaded = false;

		//preview drawing variables
		sf::Vector2u _terrainPosition;
		MutableTileMap _terrainPlacement;

		//tile placement mode variables
		tile _tileInfo;
		hades::types::uint8 _tile_draw_size = 1;

		//core map drawing variables
		sf::View _gameView;
		MutableTileMap _terrain;
	};
}

#endif // !TILES_EDITOR_HPP
