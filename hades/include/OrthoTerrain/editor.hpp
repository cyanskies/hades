#ifndef ORTHO_EDITOR_HPP
#define ORTHO_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Tiles/editor.hpp"

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
		//add selector for default tile or generator
		void NewLevelDialog() override;
		void NewLevel() override;
		void SaveLevel() const override;
		void LoadLevel() override;

		void SaveTiles(level &l) const;
		void LoadTiles(const level &l);

		void DrawPreview(sf::RenderTarget &target) const override;

		//New Map Settings
		editor::TileGenerator TileGenerator = editor::TileGenerator::FILL;

		//core map variables
		MutableTileMap Map;
		const resources::tile_settings *TileSettings = nullptr;

	private:
		void _enterTileMode();
		void _addTilesToUi();
		void _addTiles(const std::vector<tile> &tiles);
		void _setCurrentTile(tile t);

		//gui
		sfg::Box::Ptr _tileWindow = nullptr;

		//preview drawing variables
		sf::Vector2i _tilePosition;
		draw_size_t _tileDrawSize = 1;
		MutableTileMap _tilePreview;
		editor::TileEditMode _tileMode = editor::TileEditMode::NONE;

		//selected tile for drawing
		//also holds the default for filling a new map
		tile _tileInfo;
	};
}

#endif // !ORTHO_EDITOR_HPP
