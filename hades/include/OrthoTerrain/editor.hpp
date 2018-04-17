#ifndef ORTHO_EDITOR_HPP
#define ORTHO_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Tiles/editor.hpp"

#include "OrthoTerrain/resources.hpp"

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

	protected:
		void FillToolBar(AddToggleButtonFunc, AddButtonFunc, AddSeperatorFunc) override;

		void Terrainset(const resources::terrainset*);
		
	private:
		void _enterTerrainMode();
		void _addTerrainToGui();
		void _addTerrain(const std::vector<const resources::terrain*> &terrain);
		void _setCurrentTerrain(const resources::terrain*);

		const resources::terrainset *_terrainset;
		editor::TerrainEditMode _terrainMode = editor::TerrainEditMode::NONE;

		sfg::Box::Ptr _terrainWindow = nullptr;
	};
}

#endif // !ORTHO_EDITOR_HPP
