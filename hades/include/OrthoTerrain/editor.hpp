#ifndef ORTHO_EDITOR_HPP
#define ORTHO_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "OrthoTerrain/terrain.hpp"
#include "Tiles/editor.hpp"

//a simple terrain editor
namespace ortho_terrain
{
	namespace editor
	{
		const hades::types::string terrain_selector_panel = "terrain-selector";

		enum EditMode {
			TERRAIN = tiles::editor::TILE_EDIT_END + 1, //draw a terrain tile with transition fixups
			TERRAIN_EDIT_END
		};
	}

	class terrain_editor : public tiles::tile_editor_t<MutableTerrainMap>
	{
	public:
		void generate() override;
		///void update(sf::Time deltaTime, const sf::RenderTarget&, hades::InputSystem::action_set) override;
		void draw(sf::RenderTarget &target, sf::Time deltaTime) override;

	private:
		void FillTileSelector() override;

		void GenerateDrawPreview(const sf::RenderTarget&, const hades::InputSystem::action_set&) override;
		void TryDraw(const hades::InputSystem::action_set&) override;

	private:
		//preview drawing variables
		sf::Vector2u _terrainPosition;
		MutableTerrainMap _terrainPreview;

		const resources::terrain *_terrainInfo = nullptr;
		hades::types::uint8 _terrain_draw_size = 1;
	};
}

#endif // !ORTHO_EDITOR_HPP
