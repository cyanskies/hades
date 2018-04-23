#ifndef ORTHO_EDITOR_HPP
#define ORTHO_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Tiles/editor.hpp"

#include "OrthoTerrain/resources.hpp"
#include "OrthoTerrain/terrain.hpp"

namespace ortho_terrain
{
	namespace editor
	{
		enum EditMode : objects::editor::EditMode_t
		{
			TERRAIN = tiles::editor::EditMode::TILE_MODE_END, TERRAIN_MODE_END
		};

		enum class TerrainEditMode {
			NONE, //no tile selected
			TERRAIN, //draw a specific tile, no cleanup
		};
	}

	class terrain_editor : public tiles::tile_editor
	{
	public:
		void init() override;
		void draw(sf::RenderTarget &target, sf::Time deltaTime) override;

	protected:
		void FillToolBar(AddToggleButtonFunc, AddButtonFunc, AddSeperatorFunc) override;
		void GenerateDrawPreview(const sf::RenderTarget&, MousePos) override;
		void OnModeChange(EditMode_t t) override;
		void OnClick(const sf::RenderTarget&, MousePos) override;
		void NewLevel() override;
		void DrawPreview(sf::RenderTarget &target) const override;

		void DrawTerrain(sf::RenderTarget &target);

		void Terrainset(const resources::terrainset*);
		
	private:
		void _enterTerrainMode();
		void _addTerrainToGui();
		void _addTerrain(const std::vector<const resources::terrain*> &terrain);
		void _setCurrentTerrain(const resources::terrain*);

		const resources::terrain *_terrain = nullptr;
		const resources::terrainset *_terrainset = nullptr;
		editor::TerrainEditMode _terrainMode = editor::TerrainEditMode::NONE;

		sf::Vector2i _drawPosition;
		MutableTerrainMap _map;
		MutableTerrainMap _preview;

		sfg::Box::Ptr _terrainWindow = nullptr;
	};
}

#endif // !ORTHO_EDITOR_HPP
