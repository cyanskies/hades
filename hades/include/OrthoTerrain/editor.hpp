#ifndef ORTHO_EDITOR_HPP
#define ORTHO_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "OrthoTerrain/terrain.hpp"
#include "OrthoTerrain/serialise.hpp"

//a simple terrain editor

namespace ortho_terrain
{
	namespace editor
	{
		//screen view height
		const hades::types::int32 view_height = 240;

		enum class EditMode {
			NONE, //no editing
			TILE, //draw a specific tile, no cleanup
			TERRAIN, //draw a terrain tile with transition fixups
		};
	}

	class terrain_editor : public hades::State
	{
	public:
		terrain_editor() = default;
		explicit terrain_editor(const hades::types::string&);
		//generate random map if not already
		void init() override; 

		void generate();
		
		void load(const OrthoSave&);
		OrthoSave save_terrain() const;
		
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
		void FillTileList(const std::vector<hades::data::UniqueId> &terrains,
			const std::vector<hades::data::UniqueId> &tilesets);

		hades::types::string Mod, Filename;

	private:
		void _generatePreview(ortho_terrain::resources::terrain *terrain, sf::Vector2u vposition, hades::types::uint8 size);

		void _new(const hades::types::string&, const hades::types::string&, tile_count_t width, tile_count_t height);
		void _load(const hades::types::string&, const hades::types::string&);
		void _save() const;

		//editor settings
		editor::EditMode _editMode = editor::EditMode::NONE;
		ortho_terrain::resources::terrain_settings *_tile_settings;
		//default save is a file called new.lvl that stored alongside the mod archives
		bool _loaded = false;

		//shared drawing variables
		sf::Vector2u _terrainPosition;
		ortho_terrain::EditMap _terrainPlacement;

		//tile placement mode variables
		ortho_terrain::tile _tileInfo;
		hades::types::uint8 _tile_draw_size = 1;

		//terrain placement mode variables
		ortho_terrain::resources::terrain *_terrainInfo = nullptr;
		hades::types::uint8 _terrain_draw_size = 1;
	
		sf::View _gameView;
		ortho_terrain::EditMap _terrain;
	};
}

#endif // !ORTHO_EDITOR_HPP
